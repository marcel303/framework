/*
 *  Sun.cpp
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Sun.h"

#include "Rendering.h"

#define DIST 150
void Sun::Update()
{
	CalcHeadings();
}

void Sun::CalcHeadings()
{
	Vec2 vec;
	float angle = 0;
	for(int i = 0; i < 7; i++)
	{
		angle = PI * 0.25f * (i+1.0f);
		vec.x = m_shape.pos.x + cos(m_dir + angle)*DIST;
		vec.y = m_shape.pos.y + sin(m_dir + angle)*DIST;
		m_targetPositions[i] = vec;
	}
	
}


void Sun::Render()
{
	m_dir = 0;
	Entity::Render();
	RenderCircle(m_shape.pos.x + cos(m_dir)*DIST, m_shape.pos.y + sin(m_dir)*DIST, 4);
	
	for(int i = 0; i < 7; i++)
	{
		RenderCircle(m_targetPositions[i].x, m_targetPositions[i].y, 4);
	}
}

