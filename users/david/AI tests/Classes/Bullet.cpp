/*
 *  Bullet.cpp
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Bullet.h"

#include "World.h"

#include "Defines.h"



Bullet::Bullet() : Entity()
{
	m_life = BULLETRANGE; 
	m_vel = Vec2(); 
	m_shape.rad = 4;
	m_mass = 0.1;
}


void Bullet::Update()
{
	m_shape.pos += m_vel * 0.1f;
	m_life --;
	
	if(m_life < 1)
		World::I().AddToBulletPool(this);
}

void Bullet::Fire(const Vec2& vel, const Vec2& pos)
{
	m_shape.pos = pos;
	m_vel = vel;
	m_life = BULLETRANGE;
}
