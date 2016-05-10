#ifndef LASERPACKET_H
#define LASERPACKET_H

class Packet
{
public:
	enum EventType
	{
		PlayerId,
		Health,
		Shoot,
		Shot,
		Weapon,
		Kills,
		Deaths,
		IsAlive,
		MaxHealth,
		MaxAmmo,
		InitialData,
                Heartbeat
	};
	
	EventType Event;
	int Data1;
	int Data2;
	int Data3;
	int HashCheck;
	
	Packet() { }
	
	Packet(EventType _e, int _Data1, int _Data2, int _Data3)
	{
		Event = _e;
		Data1 = _Data1;
		Data2 = _Data2;
		Data3 = _Data3;
		HashCheck = Hash();
	}
	
	int Hash()
	{
		int hashCode = (int)Event;
		hashCode = (hashCode * 397) ^ Data1;
		hashCode = (hashCode * 397) ^ Data2;
		hashCode = (hashCode * 397) ^ Data3;
		HashCheck = hashCode;
		return hashCode;
	}
	
	bool IsValidPacket()
	{
		return HashCheck == Hash();
	}
	
	~Packet() { }
};

#endif
