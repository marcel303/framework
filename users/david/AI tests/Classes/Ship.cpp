/*
 *  Ship.cpp
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Ship.h"

#include "World.h"

#include "Rendering.h"

Ship::Ship()
{
	Reset();	
	//m_target = &World::I().sun;
}

Ship::~Ship()
{
}

void Ship::Reset()
{
	m_vel = Vec2(0.1f, 0.0f);
	m_dir = Vec2(cos(0), sin(0));
	
	m_angle = 0;
	
	m_energy = 10;
	
	m_mass = 0.1;
	m_boost = false;
	
	m_state = SEARCH;
	
	//m_squad = 0;
}

void Ship::Init(const Vec2& pos, float rad)
{
	Reset();
	
	m_shape.Set(pos, rad);
	m_avoidShape.Set(pos, rad*2);
}

void Ship::Update()
{
	if(World::I().FighterAvoidRadius.Hit(m_shape))
		m_avoidSun = true;
	m_energy +=1;
	if(m_energy > 10) 
		m_energy = 10;
	
	m_shape.pos += m_vel  * 0.1f;
	m_avoidShape.Set(m_shape.pos, m_shape.rad*4);
	
	Vec2 dist =(m_target->m_shape.pos - m_shape.pos); 
	
	switch(m_state)
	{
			
		case CLOSEIN:
		{
			if(World::I().FighterAttackRadius.Hit(m_shape)) //attackrange
			{
				m_state = ATTACK;
				
				m_targetHeading = m_target->m_shape.pos;
				
			}
			CloseIn();
			
			break;
		}
			
		case ATTACK:
		{
			if(m_avoidSun)
				AvoidSun();
			else
			{
				if(dist.qLen() < SHIPRANGE*SHIPRANGE)
					Attack();
				else
					CloseIn();
			}
			
			break;
		}
			
		case SEARCH:
		{
			Vec2 heading = World::I().m_sun.m_targetPositions[0];
			float dist = (heading - m_shape.pos).qLen();
			for(int i = 1; i < 7; i++)
			{
				
				if((World::I().m_sun.m_targetPositions[i] - m_shape.pos).qLen() < dist)
				{
					dist = (World::I().m_sun.m_targetPositions[i] - m_shape.pos).qLen();
					heading = World::I().m_sun.m_targetPositions[i];
				}
			}
			m_targetHeading = heading;
			
			m_state = CLOSEIN;
			break;
		}
			
		default:
			break;
	}
	
	
	
	
	AvoidHoles();
	AvoidFriendlies();
	
	
	
	
	//m_result.Normalize() * SHIPSPEED;
	//	if(m_boost)
	//		m_result *= 2;
	//	
	//	m_vel += m_result;
	//	
	//	m_vel -= m_vel*RESISTANCE;
	//	m_dir = m_vel;
	//	m_dir.Normalize();
	
	m_result.Normalize();
	
	Vec2 d1 = m_dir;
	Vec2 d2 = m_result;//m_target->m_shape.pos - m_shape.pos;
	
	int angleDiff = toDeg(atan2(d2.y, d2.x)-atan2(d1.y, d1.x));
	if (angleDiff > 180)
		angleDiff -= 360;
	if (angleDiff < -180)
		angleDiff += 360;
	
	int angleSpeed = 2;
	
	if (abs(angleDiff) < angleSpeed)
		m_angle += angleDiff;
	else
	{
		if (angleDiff > 0)
			m_angle+=angleSpeed;
		if (angleDiff < 0)
			m_angle-=angleSpeed;
	}
	
	
	m_dir.x = cos(toRad(m_angle));
	m_dir.y = sin(toRad(m_angle));
	
	//m_vel += m_result;
	float mult = SHIPSPEED;
	if(m_boost)
		mult *= BOOST;
	
	m_vel += m_dir*mult;
	
	
	m_vel -= m_vel*RESISTANCE;
	//m_dir = m_vel;
	//m_dir.Normalize();
}

void Ship::AvoidSun()
{
	if((m_target->m_shape.pos - m_shape.pos).qLen() > World::I().FighterAttackRadius.rad*World::I().FighterAttackRadius.rad)
		m_avoidSun = false;
	
	Vec2 b =(m_targetHeading - m_shape.pos);
	//Vec2 b =(m_target->m_shape.pos - m_shape.pos);
	
	LineQueue(m_shape.pos.x, m_shape.pos.y, m_shape.pos.x - b.x, m_shape.pos.y);
	
	b.Normalize();
	m_result -= b; //todo weight
	
}

void Ship::Attack()
{
	if(m_energy > 9)
	{
		
		Vec2 b =(m_target->m_shape.pos - m_shape.pos); 
		if(toDeg(acos(m_dir.Dot(b.Normalize()))) <30.0f)
		{
			World::I().FireBullet(m_dir*BULLETSPEED, m_shape.pos);
			m_energy -=10;
		}
	}
}


void Ship::CloseIn()
{
	Vec2 b =(m_targetHeading - m_shape.pos);
	//Vec2 b =(m_target->m_shape.pos - m_shape.pos);
	
	LineQueue(m_shape.pos.x, m_shape.pos.y, m_shape.pos.x - b.x, m_shape.pos.y);
	
	b.Normalize();
	
	m_result += b; //todo weight
	
	
}


void Ship::AvoidHoles()
{
	BlackHole* hitlist[MAXHITLIST];
	
	int x = 0;
	for(int i = 0; i < World::I().m_holecount; i++)
	{
		if(World::I().m_holes[i].m_suckRadius.Hit(m_shape))
		{
			hitlist[x] = &World::I().m_holes[i];
			x++;
		}
		if(x > MAXHITLIST-1)
			break;
	}
	for(int i = 0;  i < x; i++)
	{
		Vec2 b =(hitlist[i]->m_shape.pos - m_shape.pos);
		float l = b.Len();
		
		LineQueue(m_shape.pos.x, m_shape.pos.y, m_shape.pos.x - b.x * (10/l), m_shape.pos.y - b.y * (10/l));
		
		b.Normalize();
		m_result -= b;//; * (10/l); //TODO: WEIGHT
		
		
	}
	
	//list of 5
	//reverse heading
}

void Ship::AvoidFriendlies()
{
	//if(m_squad)
	//{
	for(int i = 0; i < SHIPS; i++)
	{
		if(&World::I().m_ships[i] != this)
		{
			if(World::I().m_ships[i].m_avoidShape.Hit(m_avoidShape))
			{
				Vec2 a(World::I().m_ships[i].m_shape.pos - m_shape.pos);
				//float l = a.Len();
				
				LineQueue(m_shape.pos.x, m_shape.pos.y, m_shape.pos.x - a.x, m_shape.pos.y - a.y);
				
				a.Normalize();
				m_result -= a;
				
				
			}
		}
	}
}

void Ship::SetTarget(Entity* tar)
{
	m_target = tar;
}
void Ship::SetTargetHeading(const Vec2& heading)
{
	m_targetHeading = heading;
}

//void Ship::SetSquad(Squad* squad)
//{
//	m_squad = squad;
//}

void Ship::Render()
{
	Entity::Render();
	RenderLine(m_shape.pos.x, m_shape.pos.y, m_shape.pos.x+m_dir.x*30, m_shape.pos.y+m_dir.y*30);
	
	RenderCircle(m_avoidShape.pos.x, m_avoidShape.pos.y, m_avoidShape.rad);
	
	//RenderLine(m_shape.pos.x, m_shape.pos.y + 10.0f, m_shape.pos.x + dir1.x * 100.0f, m_shape.pos.y + dir1.y * 100.0f +10.0f);
	//RenderLine(m_target->m_shape.pos.x, m_target->m_shape.pos.y + 10.0f, m_target->m_shape.pos.x + dir2.x * 100.0f, m_target->m_shape.pos.y + dir2.y * 100.0f +10.0f);
	
	//[[NSString stringWithFormat:@"%f", angle1] drawAtPoint:CGPointMake(0.0f, 0.0f) withFont: [UIFont systemFontOfSize:12]];
	//	
	//	std::string bla3 = toString(blaat1);
	//	NSString* temp3 = [NSString stringWithCString:bla3.c_str()];
	//	[temp3 drawAtPoint:CGPointMake(100.0f, 40.0f) withFont: [UIFont systemFontOfSize:12]];
	
	//CGContextRef myContext = UIGraphicsGetCurrentContext();
	//CGAffineTransform rotate = CGAffineTransformMakeRotation(m_angle);
	//
	//CGContextDrawImage(myContext, CGRectMake(0, 0, 480, 320), [World::I().img CGImage]);
	
}
