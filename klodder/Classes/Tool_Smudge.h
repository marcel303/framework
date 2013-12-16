#pragma once

#include "ITool.h"

class ToolSettings_SmudgeSoft
{
public:
	ToolSettings_SmudgeSoft()
	{
		diameter = 15;
		hardness = 0.0f;
		spacing = 0.05f;
		strength = 1.0f;
	}
	
	ToolSettings_SmudgeSoft(int _diameter, float _hardness, float _spacing, float _strength)
	{
		diameter = _diameter;
		hardness = _hardness;
		spacing = _spacing;
		strength = _strength;
	}
	
	int diameter;
	float hardness;
	float spacing;
	float strength;
};

class ToolSettings_SmudgePattern
{
public:
	ToolSettings_SmudgePattern()
	{
		diameter = 15;
		patternId = 0;
		spacing = 0.05f;
		strength = 1.0f;
	}
	
	ToolSettings_SmudgePattern(int _diameter, uint32_t _patternId, float _spacing, float _strength)
	{
		diameter = _diameter;
		patternId = _patternId;
		spacing = _spacing;
		strength = _strength;
	}
	
	int diameter;
	uint32_t patternId;
	float spacing;
	float strength;
};

class Tool_Smudge : public ITool
{
public:
	Tool_Smudge();

	void Setup(float strength);

	void Apply(Bitmap* bmp, const Filter* filter, float x, float y, float dx, float dy, AreaI& dirty);

private:
	float mStrength;
};
