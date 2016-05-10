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
	
	int32_t Event;
	int32_t Data1;
	int32_t Data2;
	int32_t Data3;
	int32_t HashCheck;
	
	Packet() { }
	
	Packet(EventType _e, int _Data1, int _Data2, int _Data3)
	{
		Event = (int32_t) _e;
		Data1 = _Data1;
		Data2 = _Data2;
		Data3 = _Data3;
		HashCheck = Hash();
	}
	
	int32_t Hash()
	{
		int32_t hashCode = Event;
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
