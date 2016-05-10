#include <LiquidCrystal.h>
#include <WiFiShieldOrPmodWiFi_G.h>
#include <DNETcK.h>
#include <DWIFIcK.h>

#include "packet.h"
#include "player.h"

#define INT2 2
#define pinINT2 7
#define INT3 3
#define pinINT3 8

LiquidCrystal lcd(26, 27, 28, 29, 30, 31, 32);

char * szIPServer = "192.168.43.151";    // server to connect to
unsigned short portServer = 52737;     // port 44300
const char * szSsid = "Wizzy";
const char * szPassPhrase = "21101994";
#define WiFiConnectMacro() DWIFIcK::connect(szSsid, szPassPhrase, &status)

typedef enum
{
    NONE = 0,
    WRITE,
    READ,
    CLOSE,
    DONE,
    WAIT
} STATE;

STATE state = WRITE;

TcpClient tcpClient;
byte rgbRead[1024];
int cbRead = 0;

byte writeBuffer[50];
int writeLength = 0;
bool pendingWrite = false;

unsigned long lastScreenRefresh = 0;

Player player;

byte ammo[8] = {
	0b00100,
	0b01110,
	0b01110,
	0b01110,
	0b01110,
	0b11111,
	0b00000,
	0b11111
};

byte heart[8] = {
	0b00000,
	0b01010,
	0b11111,
	0b11111,
	0b11111,
	0b01110,
	0b00100,
	0b00000
};

void setup() 
{
    Serial.begin(4800);
    
    randomSeed(analogRead(0));
    
    pinMode(pinINT2, INPUT);
    pinMode(pinINT3, INPUT);
    attachInterrupt(INT2, btn0Push, RISING);
    attachInterrupt(INT3, btn1Push, RISING);
    
    lcd.createChar(0, heart);
    lcd.createChar(1, ammo);
    
    lcd.begin(16, 2);
    
    lcd.print("Start");
    
    DNETcK::STATUS status;
    int conID = DWIFIcK::INVALID_CONNECTION_ID;
    
    if((conID = WiFiConnectMacro()) != DWIFIcK::INVALID_CONNECTION_ID)
    {
        lcd.setCursor(0, 0);
        lcd.print("Wifi Connected");
        state = READ;
    }
    else
    {
        lcd.setCursor(0, 0);
        lcd.print("Wifi Fail");
        state = CLOSE;
    }
    
    // use DHCP to get our IP and network addresses
    DNETcK::begin();

    // make a connection to our echo server
    tcpClient.connect(szIPServer, portServer);
} 

void sendPacket(Packet& pkt)
{
    memcpy(writeBuffer, &pkt, sizeof(pkt));
    writeLength = sizeof(pkt);
    pendingWrite = true;
}

byte laserBuffer[50];
int laserLength = 0;
unsigned long laserStart = 0;
bool pendingLaser = false;

void writeLaserData(Packet& pkt)
{
    memcpy(laserBuffer, &pkt, sizeof(pkt));
    laserLength = sizeof(pkt);
    pendingLaser = true;
    laserStart = millis();
}

void sendLaserData()
{
    if(pendingLaser)
    {
        Serial.print('L');
        Serial.write(laserBuffer, laserLength);
        
        if(millis() > laserStart + 500)
            pendingLaser = false;
    }
}

void btn0Push()
{
    if(tcpClient.isConnected() && player.canFire())
    {
        player.ammo--;
        
        Packet pkt(Packet::Shoot, player.playerID, random(1000000), 0);
        sendPacket(pkt);
        writeLaserData(pkt);
    }
}

void btn1Push()
{
    if(tcpClient.isConnected())
    {
        player.ammo = player.maxAmmo;
    }
}
 
void loop() 
{
    switch(state)
    {
       case WRITE:
            if(tcpClient.isConnected())
            {
                //tcpClient.writeStream(rgbWriteStream, cbWriteStream);
            }
            break;
            
         case WAIT:
             if(tcpClient.isConnected())
                 state = READ;
             break;
         case READ:
            if(tcpClient.isConnected())
            {
                if(pendingWrite)
                {
                    tcpClient.writeStream(writeBuffer, writeLength);
                    pendingWrite = false;
                }
                
                if((cbRead = tcpClient.available()) > 0)
                {
                    cbRead = cbRead < sizeof(rgbRead) ? cbRead : sizeof(rgbRead);
                    cbRead = tcpClient.readStream(rgbRead, cbRead);
                    
                    Packet pkt;
                    if(cbRead >= sizeof(pkt))
                    {
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
                                    break;
                                    
                                case Packet::Deaths:
                                    player.deaths = pkt.Data1;
                                    break;
                                    
                                case Packet::Kills:
                                    player.kills = pkt.Data1;
                                    break;
                                    
                                case Packet::IsAlive:
                                    player.isAlive = true;
                                    break;

                               case Packet::Heartbeat:
                                   Packet hpkt(Packet::Heartbeat, 0, 0, 0);
                                   sendPacket(hpkt);
                                   break;
                            }
                        }
                    }
                }
            } else {
                tcpClient.connect(szIPServer, portServer);
            }
            break;

        case CLOSE:
            tcpClient.close();
            state = DONE;
            break;

        case DONE:
        default:
            break;
    }
    
    unsigned long time = millis();
    
    if(tcpClient.isConnected() && time > lastScreenRefresh + 250)
    {
        lcd.clear();
        lcd.write((uint8_t)0);
        lcd.print("  ");
        lcd.write((uint8_t)1);
        lcd.print("  K  D  ");
        
        lcd.setCursor(0, 1);
        lcd.print(player.health, DEC);
        
        lcd.setCursor(3, 1);
        lcd.print(player.ammo, DEC);
        
        lcd.setCursor(6, 1);
        lcd.print(player.kills, DEC);
        
        lcd.setCursor(9, 1);
        lcd.print(player.deaths, DEC);
        
        lastScreenRefresh = time;
    }
    
    sendLaserData();

    // keep the stack alive each pass through the loop()
    DNETcK::periodicTasks(); 
}
