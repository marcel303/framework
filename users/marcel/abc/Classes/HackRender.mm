#include "HackRender.h"

static Vec2F sTranslation;

void HR_SetTranslation(Vec2F translation)
{
	sTranslation = translation;
}

void HR_Circle(Vec2F position, float radius)
{
	position += sTranslation;
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	CGRect rect = CGRectMake(position[0] - radius, position[1] - radius, radius * 2.0f, radius * 2.0f);
	
	CGContextFillEllipseInRect(ctx, rect);
}

void HR_Line(Vec2F position1, Vec2F position2)
{
	position1 += sTranslation;
	position2 += sTranslation;
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	CGPoint points[] = 
	{ 
		CGPointMake(position1[0], position1[1]),
		CGPointMake(position2[0], position2[1])
	};
		
	CGContextStrokeLineSegments(ctx, points, 2);
}

void HR_Rect(Vec2F position1, Vec2F position2)
{
	position1 += sTranslation;
	position2 += sTranslation;
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	CGRect rect = CGRectMake(position1[0], position1[1], position2[0] - position1[0], position2[1] - position1[1]);
	
	CGContextFillRect(ctx, rect);
}

void HR_RectLine(Vec2F position1, Vec2F position2)
{
	position1 += sTranslation;
	position2 += sTranslation;
	
	CGContextRef ctx = UIGraphicsGetCurrentContext();
	
	CGRect rect = CGRectMake(position1[0], position1[1], position2[0] - position1[0], position2[1] - position1[1]);
	
	CGContextStrokeRect(ctx, rect);
}
