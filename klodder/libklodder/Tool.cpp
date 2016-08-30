#include "Bitmap.h"
#include "Filter.h"
#include "Tool.h"

Tool::~Tool()
{
}

void Tool::Apply(Bitmap * bmp, const Filter * filter, const float x, const float y, const float dx, const float dy, AreaI & dirty)
{
	throw ExceptionNA();
}

void Tool::ApplyFilter(Filter * bmp, const Filter * filter, const float x, const float y, const float dx, const float dy, AreaI & dirty)
{
	throw ExceptionNA();
}

void Tool::ApplyFilter(Bitmap * bmp, const Filter * filter, const float x, const float y, const float dx, const float dy, const Rgba & color, AreaI & dirty)
{
	throw ExceptionNA();
}
