/*
 *  Rendering.h
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#pragma once




#include <deque>
#include <string>
#include <sstream>

template <class T>
inline std::string toString (const T& t)
{
	std::stringstream ss;
	ss << t;
	return ss.str();
}


template <class T>
inline bool fromString(T& t, const std::string& s, std::ios_base& (*f)(std::ios_base&))
{
	std::istringstream iss(s);
	return !(iss >> f >> t).fail();
}



struct Line
{
	float x1, x2, y1, y2;
};
static std::deque<Line> lineq;
void LineQueue(float x1, float y1, float x2, float y2);
void RenderLine(float x1, float y1, float x2, float y2);
void RenderLineQ();
void RenderCircle(float posx, float posy, float r);



