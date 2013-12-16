#pragma once

#include "Shape.h"

class ShapeIO
{
public:
	static int LoadVG(const char* fileName, Shape& shape, IImageLoader* imageLoader);
	static int SaveVG(const Shape& shape, char* fileName);

//	static int SaveVGC(const Shape& shape, char* fileName);
};
