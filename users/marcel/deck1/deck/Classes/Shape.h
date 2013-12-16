#pragma once

#include "Types.h"

const static int MAX_SIDES = 6;

enum ShapeType
{
	ShapeType_Undefined,
	ShapeType_Convex,
	ShapeType_Circle
};

class Shape
{
public:
	Shape();
	
	void Make_Convex(int sides, float radius);
	void Make_Circle(float radius);
	void Make_Rect(int x, int y, int sx, int sy);
	
	struct
	{
		float radius;
	} circle;
	struct
	{
		Vec2F coords[MAX_SIDES];
		int coordCount;
	} convex;
	
	ShapeType type;
	bool fixed;
};
