/*
 *  Entity.h
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "Math.h"

class Entity
{
public:
	Entity();
	virtual ~Entity(){}
	
	virtual void Update() =0;
	void SetMass(float mass){m_mass = mass;}
	virtual void Render();
	
	Rad m_shape;
	float m_mass;
};