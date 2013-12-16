/*
 *  Sun.h
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once

#include "Entity.h"

class Sun : public Entity
{
public:
	Sun():Entity(){m_shape.pos = Vec2(160,240); m_shape.rad = 32;}
	virtual ~Sun(){}
	
	virtual void Update();	
	virtual void Render();
	void CalcHeadings();
	
	float m_dir;
	Vec2 m_targetPositions[7];
	
	Vec2 m_turretPos;
	
	
	
};