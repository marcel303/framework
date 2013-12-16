#pragma once

#include "klodder_forward.h"
#include "Types.h"

#if defined(IPAD)
#define MAX_BRUSH_RADIUS 255
#else
#define MAX_BRUSH_RADIUS 127
#endif

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

class ITool
{
public:
	virtual ~ITool();
	
	virtual void Apply(Bitmap* bmp, const Filter* filter, float x, float y, float dx, float dy, AreaI& dirty);
	virtual void ApplyFilter(Filter* bmp, const Filter* filter, float x, float y, float dx, float dy, AreaI& dirty);
	virtual void ApplyFilter(Bitmap* bmp, const Filter* filter, float x, float y, float dx, float dy, const Rgba& color, AreaI& dirty);
};
