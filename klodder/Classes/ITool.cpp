#include "Bitmap.h"
#include "Filter.h"
#include "ITool.h"

ITool::~ITool()
{
}

void ITool::Apply(Bitmap* bmp, const Filter* filter, float x, float y, float dx, float dy, AreaI& dirty)
{
	throw ExceptionNA();
}

void ITool::ApplyFilter(Filter* bmp, const Filter* filter, float x, float y, float dx, float dy, AreaI& dirty)
{
	throw ExceptionNA();
}

void ITool::ApplyFilter(Bitmap* bmp, const Filter* filter, float x, float y, float dx, float dy, const Rgba& color, AreaI& dirty)
{
	throw ExceptionNA();
}
