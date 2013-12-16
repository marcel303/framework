#pragma once

#include "Filter.h"
#include "libgg_forward.h"

class Brush_Pattern
{
public:
	Brush_Pattern();

	void Setup(uint32_t patternId, bool isOriented, bool isFiltered, int default_Diameter, int default_Spacing, int default_Strength);

	void Load(Stream* stream, int version, bool loadFilter);
	void Save(Stream* stream, int version);

	Brush_Pattern* Duplicate() const;

	uint32_t mPatternId;
	bool mIsOriented;
	bool mIsFiltered;
	Filter mFilter;

	int mDefaultDiameter;
	int mDefaultSpacing;
	int mDefaultStrength;
};
