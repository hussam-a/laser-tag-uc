#ifndef LASERPLAYER_H
#define LASERPLAYER_H

class Player
{
public:
	int health;
	int maxHealth;
	int maxAmmo;
	int playerID;
	int deaths;
	int kills;
	int ammo;
	bool isAlive;
	
	Player()
	{
		playerID = 0;
		health = 0;
		maxHealth = 0;
		maxAmmo = 0;
		deaths = 0;
		kills = 0;
		ammo = 0;
		isAlive = true;
	}
	
	~Player() {  }
	
	void decreaseHealth(int amount)
	{
		health -= amount;

		if (health <= 0)
		{
			health = 0;
			isAlive = false;
		}
	}
	
	bool canFire()
	{
		return (isAlive && ammo > 0);
	}
	
	void reload()
	{
		ammo = maxAmmo;
	}

	void revive()
	{
		health = maxHealth;
	}
};

#endif