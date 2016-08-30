#pragma once

#include "Filter.h"
#include "Tool.h"

class ToolSettings_BrushSoft
{
public:
	ToolSettings_BrushSoft()
	{
		diameter = 15;
		hardness = 0.0f;
		spacing = 0.05f;
	}
	
	ToolSettings_BrushSoft(const int _diameter, const float _hardness, const float _spacing)
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
	
	ToolSettings_BrushPattern(const int _diameter, const uint32_t _patternId, const float _spacing)
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
	
	ToolSettings_BrushSoftDirect(const int _diameter, const float _hardness, const float _spacing)
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
	
	ToolSettings_BrushPatternDirect(const int _diameter, const uint32_t _patternId, const float _spacing)
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
	
	ToolSettings_EraserSoft(const int _diameter, const float _hardness, const float _spacing)
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
	
	ToolSettings_EraserPattern(const int _diameter, const uint32_t _patternId, const float _spacing)
	{
		diameter = _diameter;
		patternId = _patternId;
		spacing = _spacing;
	}
	 
	int diameter;
	uint32_t patternId;
	float spacing;
};

class Tool_Brush : public Tool
{
public:
	Tool_Brush();

	void Setup(const int diameter, const float hardness, const bool isOriented);
	void Setup_Pattern(const int diameter, const Filter * filter, const bool isOriented);

	virtual void ApplyFilter(Filter * bmp, const Filter * filter, const float x, const float y, const float dx, const float dy, AreaI & dirty) override;
	void ApplyFilter_Rotated(Filter * bmp, const Filter * filter, const float x, const float y, const float angle, AreaI & dirty);
	void ApplyFilter_Cheap(Filter * bmp, const Filter * filter, const float x, const float y, const float dx, const float dy, AreaI & dirty);

	void LoadBrush(Stream * stream);

	int Diameter_get() const;
	float Hardness_get() const;
	void IsOriented_set(const bool isOriented);
	const Filter * Filter_get() const;
	Filter * Filter_getRW();

private:
	void CreateFilter();

	int mDiameter;
	float mHardness;
	bool mIsOriented;

	Filter mFilter;
};

class Tool_BrushDirect : public Tool
{
public:
	Tool_BrushDirect();

	void Setup(const int diameter, const float hardness, const bool isOriented);
	void Setup_Pattern(const int diameter, const Filter * filter, const bool isOriented);

	virtual void ApplyFilter(Bitmap * bmp, const Filter * filter, const float x, const float y, const float dx, const float dy, const Rgba & color, AreaI & dirty) override;
	void ApplyFilter_Rotated(Bitmap * bmp, const Filter * filter, const float x, const float y, const float angle, const Rgba & color, AreaI & dirty);

	int Diameter_get() const;
	float Hardness_get() const;
	void IsOriented_set(bool isOriented);
	const Filter * Filter_get() const;
	Filter * Filter_getRW();

private:
	void CreateFilter();

	int mDiameter;
	float mHardness;
	bool mIsOriented;

	Filter mFilter;
};
