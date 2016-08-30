#pragma once

#include "Tool.h"

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
	
	ToolSettings_SmudgeSoft(const int _diameter, const float _hardness, const float _spacing, const float _strength)
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
	
	ToolSettings_SmudgePattern(const int _diameter, const uint32_t _patternId, const float _spacing, const float _strength)
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

class Tool_Smudge : public Tool
{
public:
	Tool_Smudge();

	void Setup(const float strength);

	virtual void Apply(Bitmap * bmp, const Filter * filter, const float x, const float y, const float dx, const float dy, AreaI & dirty) override;

private:
	float mStrength;
};
