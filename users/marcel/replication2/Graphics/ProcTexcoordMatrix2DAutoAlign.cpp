#include "ProcTexcoordMatrix2DAutoAlign.h"

Vec3 ProcTexcoordMatrix2DAutoAlign::Generate(Vec3Arg position, Vec3Arg normal)
{
	int maxAxis = 0;

	for (int i = 1; i < 3; ++i)
		if (fabsf(normal[i]) > fabsf(normal[maxAxis]))
			maxAxis = i;

	SetAxis(ShapeBuilder::AXIS(maxAxis));

	return ProcTexcoordMatrix2D::Generate(position, normal);
}
