/*
 *  World.mm
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "World.h"

#include "Rendering.h"

void World::Update()
{
	for(int i = 0; i < SHIPS; i++)
		m_ships[i].Update();
	
	
	for(int i = 0; i < m_holecount; i++)
		m_holes[i].Update();
	
	for(int i = 0; i < BULLETS; i++)
	{
		if(m_bullets[i].Alive())
			m_bullets[i].Update();
	}
	
	m_sun.Update();
}
void World::Render()
{
	for(int i = 0; i < SHIPS; i++)
		m_ships[i].Render();
	
	
	for(int i = 0; i < m_holecount; i++)
		m_holes[i].Render();
	
	for(int i = 0; i < BULLETS; i++)
	{
		if(m_bullets[i].Alive())
			m_bullets[i].Render();
	}
	
	m_sun.Render();
	
	
	RenderLineQ();
	//RenderLine(ships[0].m_shape.pos.x, ships[0].m_shape.pos.y, sun.m_shape.pos.x, sun.m_shape.pos.y);
}

void World::FireBullet(const Vec2& vel, const Vec2& pos)
{
	if(ammo)
	{
		m_bulletPool[ammo]->Fire(vel, pos);
		ammo--;
	}
}

void World::AddToBulletPool(Bullet* b)
{
	ammo++;
	m_bulletPool[ammo] = b;
}

void World::PlaceBlackHole(int x, int y, float rad)
{
	if(m_holecount < HOLES)
	{
		m_holes[m_holecount].Init(x, y, rad);
		m_holecount+=1;
	}
	//holes[0].m_shape.rad = rand()%20;
}

World::World()
{
	//img = [[UIImage alloc] initWithContentsOfFile:[[NSBundle mainBundle] pathForResource:@"Grunt" ofType:@"png"]];
	
	//Squad* s = new Squad();
	for(int i = 0; i < SHIPS; i++)
	{
		Vec2 a (rand()%320,  rand()%320);
		m_ships[i].Init(a, 8);
		
		m_ships[i].SetTarget(&m_sun);
	}
	
	
	//for(int i = 0; i < HOLES; i++)
	//	{
	//		holes[i].Init(rand()%320, rand()%320, rand()%20);
	//	}
	
	ammo = BULLETS-1;
	
	m_holecount = 0;
	
	for(int i = 0; i < BULLETS; i++)
	{
		m_bulletPool[i] = &m_bullets[i];
		m_bullets[i].m_life = 0;
	}
	
	
	FighterAttackRadius = m_sun.m_shape;
	FighterAttackRadius.rad = 200;
	FighterAvoidRadius.rad = 100;;
	FighterAvoidRadius = m_sun.m_shape;
	
	
	
	//s->AddShip(&m_ships[0]);
	//	s->AddShip(&m_ships[1]);
	//	s->AddShip(&m_ships[2]);
	//	s->AddShip(&m_ships[3]);
	//	s->AddShip(&m_ships[4]);
	
	m_ships[0].m_shape.pos = Vec2(0, 0);
	
	m_sun.CalcHeadings();
	
	
}
World::~World()
{
}