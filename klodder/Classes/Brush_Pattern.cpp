#include "Brush_Pattern.h"
#include "StreamReader.h"
#include "StreamWriter.h"

Brush_Pattern::Brush_Pattern()
{
	Setup(0, false, false, 1, 100, 100);
}

void Brush_Pattern::Setup(uint32_t patternId, bool isOriented, bool isFiltered, int default_Diameter, int default_Spacing, int default_Strength)
{
	mPatternId = patternId;
	mIsOriented = isOriented;
	mIsFiltered = isFiltered;

	mDefaultDiameter = default_Diameter;
	mDefaultSpacing = default_Spacing;
	mDefaultStrength = default_Strength;
}

void Brush_Pattern::Load(Stream* stream, int version, bool loadFilter)
{
	if (version == 1)
	{
		StreamReader reader(stream, false);

		// read settings

		mPatternId = reader.ReadUInt32();
		mIsOriented = reader.ReadUInt8() == 0 ? false : true;
		mIsFiltered = reader.ReadUInt8() == 0 ? false : true;

		if (loadFilter)
		{
			// read filter

			mFilter.Load(stream);
		}
	}
	else
	{
		throw ExceptionVA("unknown brush version: %d", (int)version);
	}
}

void Brush_Pattern::Save(Stream* stream, int version)
{
	if (version == 1)
	{
		StreamWriter writer(stream, false);

		// write settings

		writer.WriteUInt32(mPatternId);
		writer.WriteUInt8(mIsOriented ? 1 : 0);
		writer.WriteUInt8(mIsFiltered ? 1 : 0);

		// write filter

		mFilter.Save(stream);
	}
	else
	{
		throw ExceptionVA("unknown brush version: %d", (int)version);
	}
}

Brush_Pattern* Brush_Pattern::Duplicate() const
{
	Brush_Pattern* result = new Brush_Pattern();

	result->Setup(mPatternId, mIsOriented, mIsFiltered, mDefaultDiameter, mDefaultSpacing, mDefaultStrength);

	result->mFilter.Size_set(mFilter.Sx_get(), mFilter.Sy_get());

	mFilter.Blit(&result->mFilter);

	return result;
}
