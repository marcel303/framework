#pragma once

#include "Filter.h"
#include "ITool.h"

class ToolSettings_BrushSoft
{
public:
	ToolSettings_BrushSoft()
	{
		diameter = 15;
		hardness = 0.0f;
		spacing = 0.05f;
	}
	
	ToolSettings_BrushSoft(int _diameter, float _hardness, float _spacing)
	{
		diameter = _diameter;
		hardness = _hardness;
		spacing = _spacing;
	}
	
	int diameter;
	float hardness;
	float spacing;
};

class ToolSettings_BrushPattern
{
public:
	ToolSettings_BrushPattern()
	{
		diameter = 15;
		patternId = 0;
		spacing = 0.05f;
	}
	
	ToolSettings_BrushPattern(int _diameter, uint32_t _patternId, float _spacing)
	{
		diameter = _diameter;
		patternId = _patternId;
		spacing = _spacing;
	}
	
	int diameter;
	uint32_t patternId;
	float spacing;
};

class ToolSettings_BrushSoftDirect
{
public:
	ToolSettings_BrushSoftDirect()
	{
		diameter = 15;
		hardness = 0.0f;
		spacing = 0.05f;
	}
	
	ToolSettings_BrushSoftDirect(int _diameter, float _hardness, float _spacing)
	{
		diameter = _diameter;
		hardness = _hardness;
		spacing = _spacing;
	}
	
	int diameter;
	float hardness;
	float spacing;
};

class ToolSettings_BrushPatternDirect
{
public:
	ToolSettings_BrushPatternDirect()
	{
		diameter = 15;
		patternId = 0;
		spacing = 0.05f;
	}
	
	ToolSettings_BrushPatternDirect(int _diameter, uint32_t _patternId, float _spacing)
	{
		diameter = _diameter;
		patternId = _patternId;
		spacing = _spacing;
	}
	
	int diameter;
	uint32_t patternId;
	float spacing;
};

class ToolSettings_EraserSoft
{
public:
	ToolSettings_EraserSoft()
	{
		diameter = 15;
		hardness = 0.0f;
		spacing = 0.05f;
	}
	
	ToolSettings_EraserSoft(int _diameter, float _hardness, float _spacing)
	{
		diameter = _diameter;
		hardness = _hardness;
		spacing = _spacing;
	}
	
	int diameter;
	float hardness;
	float spacing;
};

class ToolSettings_EraserPattern
{
public:
	ToolSettings_EraserPattern()
	{
		diameter = 15;
		patternId = 0;
		spacing = 0.05f;
	}
	
	ToolSettings_EraserPattern(int _diameter, uint32_t _patternId, float _spacing)
	{
		diameter = _diameter;
		patternId = _patternId;
		spacing = _spacing;
	}
	 
	int diameter;
	uint32_t patternId;
	float spacing;
};

class Tool_Brush : public ITool
{
public:
	Tool_Brush();

	void Setup(int diameter, float hardness, bool isOriented);
	void Setup_Pattern(int diameter, Filter* filter, bool isOriented);

	void ApplyFilter(Filter* bmp, const Filter* filter, float x, float y, float dx, float dy, AreaI& dirty);
	void ApplyFilter_Rotated(Filter* bmp, const Filter* filter, float x, float y, float angle, AreaI& dirty);
	void ApplyFilter_Cheap(Filter* bmp, const Filter* filter, float x, float y, float dx, float dy, AreaI& dirty);

	void LoadBrush(Stream* stream);

	int Diameter_get() const;
	float Hardness_get() const;
	void IsOriented_set(bool isOriented);
	const Filter* Filter_get() const;
	Filter* Filter_getRW();

private:
	void CreateFilter();

	int mDiameter;
	float mHardness;
	bool mIsOriented;

	Filter mFilter;
};

class Tool_BrushDirect : public ITool
{
public:
	Tool_BrushDirect();

	void Setup(int diameter, float hardness, bool isOriented);
	void Setup_Pattern(int diameter, Filter* filter, bool isOriented);

	void ApplyFilter(Bitmap* bmp, const Filter* filter, float x, float y, float dx, float dy, const Rgba& color, AreaI& dirty);
	void ApplyFilter_Rotated(Bitmap* bmp, const Filter* filter, float x, float y, float angle, const Rgba& color, AreaI& dirty);

	int Diameter_get() const;
	float Hardness_get() const;
	void IsOriented_set(bool isOriented);
	const Filter* Filter_get() const;
	Filter* Filter_getRW();

private:
	void CreateFilter();

	int mDiameter;
	float mHardness;
	bool mIsOriented;

	Filter mFilter;
};
