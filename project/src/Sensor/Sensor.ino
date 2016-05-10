#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>

#include "packet.h"
#include "player.h"

#define ADAFRUIT_CC3000_IRQ   3
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10

Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT, SPI_CLOCK_DIVIDER);

#define WLAN_SSID       "Wizzy"
#define WLAN_PASS       "21101994"
#define WLAN_SECURITY   WLAN_SEC_WPA2
uint32_t connectIp = 0xC0A82B97; //192.168.43.151
uint16_t connectPort = 52737;

typedef enum
{
    NONE = 0,
    WRITE,
    READ,
    CLOSE,
    DONE,
} STATE;

STATE state = READ;
Adafruit_CC3000_Client client;

byte rgbRead[50];
int cbRead = 0;

byte laserBuffer[20];

byte writeBuffer[50];
int writeLength = 0;
bool pendingWrite = false;

Player player;

void setup() {
  Serial.begin(4800);
  Serial.println(F("Hello, CC3000!\n")); 

  displayDriverMode();
  
  Serial.println(F("\nInitialising the CC3000 ..."));
  if (!cc3000.begin()) {
    Serial.println(F("Unable to initialise the CC3000! Check your wiring?"));
    for(;;);
  }

  uint16_t firmware = checkFirmwareVersion();
  if (firmware < 0x113) {
    Serial.println(F("Wrong firmware version!"));
    for(;;);
  } 
  
  displayMACAddress();
  
  Serial.println(F("\nDeleting old connection profiles"));
  if (!cc3000.deleteProfiles()) {
    Serial.println(F("Failed!"));
    while(1);
  }

  /* Attempt to connect to an access point */
  char *ssid = WLAN_SSID;             /* Max 32 chars */
  Serial.print(F("\nAttempting to connect to ")); Serial.println(ssid);
  
  /* NOTE: Secure connections are not available in 'Tiny' mode! */
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP()) {
    delay(100); // ToDo: Insert a DHCP timeout!
  }

  /* Display the IP address DNS, Gateway, etc. */  
  while (!displayConnectionDetails()) {
    delay(1000);
  }

  client = cc3000.connectTCP(connectIp, connectPort);
}

void sendPacket(Packet& pkt)
{
    memcpy(writeBuffer, &pkt, sizeof(pkt));
    client.write(writeBuffer, sizeof(pkt));
}

void handleData(byte* buffer, int length)
{
  Packet pkt;
  
  if(length < sizeof(pkt))
    return;
  memcpy(&pkt, rgbRead, sizeof(pkt));

  if(pkt.IsValidPacket())
  {
      switch(pkt.Event)
      {
          case Packet::InitialData:
              player.playerID = pkt.Data1;
              player.maxHealth = pkt.Data2;
              player.maxAmmo = pkt.Data3;
              player.health = player.maxHealth;
              player.ammo = player.maxAmmo;

              Serial.print("Player ID: ");
              Serial.println(player.playerID);
              break;
              
          case Packet::Deaths:
              player.deaths = pkt.Data1;

              Serial.print("Deaths: ");
              Serial.println(player.deaths);
              break;
              
          case Packet::Kills:
              player.kills = pkt.Data1;

              Serial.print("Kills: ");
              Serial.println(player.kills);
              break;
              
          case Packet::IsAlive:
              player.isAlive = true;
              player.health = player.maxHealth;

              Serial.print("Revived!");
              break;

          case Packet::Heartbeat:
              Packet hpkt(Packet::Heartbeat, 0, 0, 0);
              sendPacket(hpkt);
              break;
      }
  }
}

int32_t lastDecrease = 0;

void handleDataLaser(byte* buffer, int length)
{
  Packet pkt;
  
  if(length < sizeof(pkt))
    return;
    
  memcpy(&pkt, buffer, sizeof(pkt));

  if(pkt.IsValidPacket())
  {
      Serial.print("Event ID: ");
      Serial.println(pkt.Event);
      
      switch(pkt.Event)
      {
        case Packet::Shoot:
          if(pkt.Data2 == lastDecrease)
            break;

          lastDecrease = pkt.Data2;
          
          if(player.isAlive)
          {
            player.decreaseHealth(1);
            
            Packet pkt2(Packet::Health, player.health, pkt.Data1, 0);
            sendPacket(pkt2);
          }
          break;
      }
  }
  else
  {
    Serial.println("Invalid laser packet");
  }
}

/**************************************************************************/
/*!
    @brief  Displays the driver mode (tiny of normal), and the buffer
            size if tiny mode is not being used

    @note   The buffer size and driver mode are defined in cc3000_common.h
*/
/**************************************************************************/
void displayDriverMode(void)
{
  #ifdef CC3000_TINY_DRIVER
    Serial.println(F("CC3000 is configure in 'Tiny' mode"));
  #else
    Serial.print(F("RX Buffer : "));
    Serial.print(CC3000_RX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
    Serial.print(F("TX Buffer : "));
    Serial.print(CC3000_TX_BUFFER_SIZE);
    Serial.println(F(" bytes"));
  #endif
}

/**************************************************************************/
/*!
    @brief  Tries to read the CC3000's internal firmware patch ID
*/
/**************************************************************************/
uint16_t checkFirmwareVersion(void)
{
  uint8_t major, minor;
  uint16_t version;
  
#ifndef CC3000_TINY_DRIVER  
  if(!cc3000.getFirmwareVersion(&major, &minor))
  {
    Serial.println(F("Unable to retrieve the firmware version!\r\n"));
    version = 0;
  }
  else
  {
    Serial.print(F("Firmware V. : "));
    Serial.print(major); Serial.print(F(".")); Serial.println(minor);
    version = major; version <<= 8; version |= minor;
  }
#endif
  return version;
}

/**************************************************************************/
/*!
    @brief  Tries to read the 6-byte MAC address of the CC3000 module
*/
/**************************************************************************/
void displayMACAddress(void)
{
  uint8_t macAddress[6];
  
  if(!cc3000.getMacAddress(macAddress))
  {
    Serial.println(F("Unable to retrieve MAC Address!\r\n"));
  }
  else
  {
    Serial.print(F("MAC Address : "));
    cc3000.printHex((byte*)&macAddress, 6);
  }
}


/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/
bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

void loop() {

  while(Serial.available())
  {
    int readVal = Serial.peek();

    if((char)readVal == 'L')
    {
      if(Serial.available() >= 21)
      {
        Serial.read();
        
        for(int i = 0; i < 20; i++)
        {
          laserBuffer[i] = (byte)Serial.read();
        }

        handleDataLaser(laserBuffer, 20);
      }
    }
    else
    {
      Serial.read();
    }
  }

  if(client.connected())
  {      
    while(client.available())
      rgbRead[cbRead++] = client.read();

    if(cbRead > 0)
      handleData(rgbRead, cbRead);

    cbRead = 0;
  }
  else
  {
    client = cc3000.connectTCP(connectIp, connectPort);
  }
}
