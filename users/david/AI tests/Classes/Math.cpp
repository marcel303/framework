/*
 *  Math.cpp
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Math.h"



Rad::Rad()
{
	rad = 0;
	pos = Vec2(0.0f, 0.0f);
}
Rad::~Rad()
{
}

bool Rad::Hit(Rad& r) const
{
	Vec2 p = pos - r.pos;
	float d = p.x * p.x + p.y * p.y;
	float d2 = rad + r.rad;
	return d <= d2*d2;
}
bool Rad::Hit(Vec2 v) const
{
	Vec2 p = pos - v;
	float d = p.x * p.x + p.y * p.y;
	return d <= rad*rad;
}

void Rad::Set(Vec2 pos, float r)
{
	this->pos = pos;
	rad  = r;
}
