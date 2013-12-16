/*
 *  Rendering.mm
 *  AI tests
 *
 *  Created by Narf on 7/15/09.
 *  Copyright 2009 __MyCompanyName__. All rights reserved.
 *
 */

#include "Rendering.h"

#include <CoreGraphics/CoreGraphics.h>
#include "MyView.h"

void LineQueue(float x1, float y1, float x2, float y2)
{
	Line l;
	l.x1 = x1;
	l.x2 = x2;
	l.y1 = y1;
	l.y2 = y2;
	lineq.push_back(l);
}


void RenderLine(float x1, float y1, float x2, float y2)
{
	CGPoint line[2];
	line[0] = CGPointMake(x1, y1);
	line[1] = CGPointMake(x2, y2);
	CGContextRef context = UIGraphicsGetCurrentContext();
	CGContextStrokeLineSegments(context, line, 2);
	//CGContextStrokeEllipseInRect(context, CGRectMake(posx - r, posy-r, r*2, r*2));
}

void RenderLineQ()
{
	Line l;
	while(!lineq.empty())
	{
		l = lineq.front();
		
		CGPoint line[2];
		line[0] = CGPointMake(l.x1, l.y1);
		line[1] = CGPointMake(l.x2, l.y2);
		CGContextRef context = UIGraphicsGetCurrentContext();
		CGContextStrokeLineSegments(context, line, 2);
		
		lineq.pop_front();
	}
}

void RenderCircle(float posx, float posy, float r)
{
	CGContextRef context = UIGraphicsGetCurrentContext();
	CGContextStrokeEllipseInRect(context, CGRectMake(posx - r, posy-r, r*2, r*2));
}