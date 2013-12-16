#pragma once

#include <vector>
#include "ITool.h"
#include "klodder_forward.h"

extern int gMaxBrushRadius;
extern int gMinBrushSpacing;
extern int gMaxBrushSpacing;

class BrushSettings
{
public:
	BrushSettings();
	void Validate();
	void Normalize();
	
	ToolType mToolType;
	uint32_t patternId;

	int diameter;
	float hardness;
	float spacing;
	float strength;
};

class BrushSettingsLibrary
{
public:
	BrushSettings Load(ToolType toolType, uint32_t patternId, Brush_Pattern* pattern);
	void Save(BrushSettings settings);

private:
	std::string MakeSettingName(ToolType toolType, uint32_t patternId);
};
