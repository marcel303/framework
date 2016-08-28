#include "Brush_Pattern.h"
#include "BrushSettingsLibrary.h"
#include "Calc.h"
#include "Log.h"
#include "Settings.h"
#include "StringEx.h"
#include "Tool.h"

#if 1
int gMaxBrushRadius = MAX_BRUSH_RADIUS;
#else
iny gMaxBrushRadius = 65;
#endif
int gMinBrushSpacing = 5;
int gMaxBrushSpacing = 250;

BrushSettings::BrushSettings()
{
	mToolType = ToolType_SoftBrush;
	patternId = 0;
	diameter = 0;
	hardness = 0.0f;
	spacing = 1.0f;
	strength = 0.0f;
}

void BrushSettings::Validate()
{
	Assert(diameter > 0 && diameter <= gMaxBrushRadius);
	Assert(hardness >= 0.0f && hardness <= 1.0f);
	Assert(spacing > 0.0f && spacing <= 10.0f);
	Assert(strength >= 0.0f && strength <= 1.0f);
}

void BrushSettings::Normalize()
{
	diameter = Calc::Mid(diameter, 1, gMaxBrushRadius);
	hardness = Calc::Mid(hardness, 0.0f, 1.0f);
	spacing = Calc::Mid(spacing, gMinBrushSpacing / 100.0f, gMaxBrushSpacing / 100.0f);
	strength = Calc::Mid(strength, 0.0f, 1.0f);
}

BrushSettings BrushSettingsLibrary::Load(ToolType toolType, uint32_t patternId, Brush_Pattern* pattern)
{
	std::string name = MakeSettingName(toolType, patternId);
	
	LOG_DBG("brush settings: load: %s", name.c_str());

	BrushSettings settings;

	settings.mToolType = toolType;
	settings.patternId = patternId;

	settings.diameter = gSettings.GetInt32(name + ".diameter", pattern ? pattern->mDefaultDiameter : 9);
	settings.hardness = gSettings.GetFloat(name + ".hardness", 0.5f);
	settings.spacing = gSettings.GetFloat(name + ".spacing", pattern ? pattern->mDefaultSpacing : 0.07f);
	settings.strength = gSettings.GetFloat(name + ".strength", pattern ? pattern->mDefaultStrength : 1.0f);
	
	settings.Normalize();

	return settings;
}

void BrushSettingsLibrary::Save(BrushSettings settings)
{
	std::string name = MakeSettingName(
		settings.mToolType, 
		settings.patternId);

	LOG_DBG("brush settings: save: %s", name.c_str());
	
//	settings.Validate();
	settings.Normalize();
	
	gSettings.SetInt32(name + ".diameter", settings.diameter);
	gSettings.SetFloat(name + ".hardness", settings.hardness);
	gSettings.SetFloat(name + ".spacing", settings.spacing);
	gSettings.SetFloat(name + ".strength", settings.strength);
}

std::string BrushSettingsLibrary::MakeSettingName(ToolType toolType, uint32_t patternId)
{
	if (toolType == ToolType_SoftBrush)
	{
		return String::Format("brush.soft.");
	}
	else if (toolType == ToolType_PatternBrush)
	{
		return String::Format("brush.pattern.%lu.", patternId);
	}
	if (toolType == ToolType_SoftBrushDirect)
	{
		return String::Format("brush.soft.direct.");
	}
	else if (toolType == ToolType_PatternBrushDirect)
	{
		return String::Format("brush.pattern.direct.%lu.", patternId);
	}
	else if (toolType == ToolType_SoftSmudge)
	{
		return String::Format("smudge.soft.");
	}
	else if (toolType == ToolType_PatternSmudge)
	{
		return String::Format("smudge.pattern.%lu.", patternId);
	}
	else if (toolType == ToolType_SoftEraser)
	{
		return String::Format("eraser.soft.");
	}
	else if (toolType == ToolType_PatternEraser)
	{
		return String::Format("eraser.pattern.%lu.", patternId);
	}
	else
	{
		throw ExceptionVA("unknown tool type");
	}
}
