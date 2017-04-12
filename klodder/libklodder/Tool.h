#pragma once

#include "libklodder_forward.h"
#include "Types.h"

#define MAX_BRUSH_RADIUS 255

enum ToolType
{
	ToolType_Undefined,
//	ToolType_Blur,
	ToolType_SoftBrush,
	ToolType_PatternBrush,
	ToolType_SoftBrushDirect,
	ToolType_PatternBrushDirect,
//	ToolType_Colorize,
//	ToolType_Sharpen,
	ToolType_SoftSmudge,
	ToolType_PatternSmudge,
	ToolType_SoftEraser,
	ToolType_PatternEraser
};

class Tool
{
public:
	virtual ~Tool();
	
	virtual void Apply(Bitmap * bmp, const Filter * filter, const float x, const float y, const float dx, const float dy, AreaI & dirty);
	virtual void ApplyFilter(Filter * bmp, const Filter * filter, const float x, const float y, const float dx, const float dy, AreaI & dirty);
	virtual void ApplyFilter(Bitmap * bmp, const Filter * filter, const float x, const float y, const float dx, const float dy, const Rgba & color, AreaI & dirty);
};
