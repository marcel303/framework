/*
 *  Bullet.h
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "Entity.h"

class Bullet : public Entity
{
public:
	Bullet();
	virtual ~Bullet(){}
	
	virtual void Update();
	
	bool Alive(){return m_life > 0;}
	void Fire(const Vec2& vel, const Vec2& pos);
	
	int m_life;
	Vec2 m_vel;
};
