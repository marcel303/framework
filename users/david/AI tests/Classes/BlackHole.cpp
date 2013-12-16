/*
 *  BlackHole.cpp
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "BlackHole.h"

#include "World.h"

#include "Rendering.h"

BlackHole::BlackHole()
{
}

BlackHole::~BlackHole()
{
}

void BlackHole::Init(float posx, float posy, float rad)
{
	m_shape.Set(Vec2(posx, posy), rad);
	m_strength = rad*BLACKHOLESTRMULT;
	m_suckRadius.Set(Vec2(posx, posy), m_strength);
}

BlackHole* BlackHole::Hit(const Rad& r)
{
	if(r.Hit(m_shape))
		return this;
	return 0;
}

void BlackHole::Update()
{
	GravityPull();
}

void BlackHole::Render()
{
	Entity::Render();
	RenderCircle(m_suckRadius.pos.x, m_suckRadius.pos.y, m_suckRadius.rad);
}

BlackHole* BlackHole::InsideSuck(const Rad& r)
{
	if(r.Hit(m_suckRadius))
		return this;
	return 0;
}

void BlackHole::GravityPull()
{
	for(int i = 0; i < SHIPS; i++)
	{
		if(World::I().m_ships[i].m_shape.Hit(m_suckRadius))
		{
			Vec2 x =(World::I().m_ships[i].m_shape.pos - m_suckRadius.pos);
			float l = x.Len();
			x/=l;
			if(l > 1.0f)
				World::I().m_ships[i].m_vel += -(x*(m_strength / World::I().m_ships[i].m_mass / l))*0.05f;
		}
	}
	
	for(int i = 0; i < BULLETS; i++)
	{
		Bullet* b = &World::I().m_bullets[i];
		if(b->Alive() && b->m_shape.Hit(m_suckRadius))
		{
			Vec2 x =(b->m_shape.pos - m_suckRadius.pos);
			float l = x.Len();
			x/=l;
			if(l > 1.0f)
				b->m_vel += -(x*(m_strength / b->m_mass / l))*0.05f;
			else
				b->m_life = 0;
		}
	}
	
	
	
	//World::I().bullets[i];
}
