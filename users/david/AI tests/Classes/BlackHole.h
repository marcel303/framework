/*
 *  BlackHole.h
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "Entity.h"

class BlackHole : public Entity
{
public:
	
	BlackHole();
	virtual ~BlackHole();
	
	void Init(float posx, float posy, float rad);
	
	virtual void Update();
	virtual void Render();
	
	BlackHole* Hit(const Rad& r);
	BlackHole* InsideSuck(const Rad& r);
	void GravityPull();
	
	float m_strength;
	
	Rad m_suckRadius;
	
};
