/*
 *  World.h
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "Defines.h"

#include "Entity.h"

#include "BlackHole.h"
#include "Bullet.h"
#include "Ship.h"
#include "Sun.h"


class World
{
public:
	static World& I()
	{
		static World w;
		return w;
	}
	
	void Update();
	void Render();
	
	void PlaceBlackHole(int x, int y, float rad);
	
	Ship m_ships[SHIPS];
	
	
	Bullet m_bullets[BULLETS];
	Bullet* m_bulletPool[BULLETS];
	int ammo;
	
	void FireBullet(const Vec2& vel, const Vec2& pos);
	void AddToBulletPool(Bullet* b);
	
	BlackHole m_holes[HOLES];
	
	Sun m_sun;
	
	//UIImage* img;
	
	int m_holecount;
	
	Rad FighterAttackRadius;
	Rad FighterAvoidRadius;
private:
	World();
	~World();
	
		
};

