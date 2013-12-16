#include <cmath>
#include "Calc.h"
#include "Shape.h"

Shape::Shape()
{
	type = ShapeType_Undefined;
}

void Shape::Make_Convex(int sides, float radius)
{
	type = ShapeType_Convex;
	convex.coordCount = sides;
	for (int i = 0; i < sides; ++i)
	{
		float angle = i * Calc::m2PI / sides;
		float x = std::cos(angle) * radius;
		float y = std::sin(angle) * radius;
		convex.coords[i].Set(x, y);
	}
}

void Shape::Make_Circle(float radius)
{
	type = ShapeType_Circle;
	circle.radius = radius;
}

void Shape::Make_Rect(int x, int y, int sx, int sy)
{
	type = ShapeType_Convex;
	convex.coordCount = 4;
	convex.coords[0].Set(x, y);
	convex.coords[1].Set(x + sx, y);
	convex.coords[2].Set(x + sx, y + sy);
	convex.coords[3].Set(x, y + sy);
}