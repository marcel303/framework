#include "StreamReader.h"
#include "StreamWriter.h"
#include "Swatch.h"

// Swatch

Swatch::Swatch()
{
}

MacRgba Swatch::Color_get() const
{
	return mColor;
}

void Swatch::Color_set(MacRgba rgba)
{
	mColor = rgba;
}

void Swatch::Read(Stream* stream)
{
	StreamReader reader(stream, false);
	
	mColor.rgba[0] = reader.ReadUInt8();
	mColor.rgba[1] = reader.ReadUInt8();
	mColor.rgba[2] = reader.ReadUInt8();
	mColor.rgba[3] = reader.ReadUInt8();
}

void Swatch::Write(Stream* stream) const
{
	StreamWriter writer(stream, false);
	
	writer.WriteUInt8(mColor.rgba[0]);
	writer.WriteUInt8(mColor.rgba[1]);
	writer.WriteUInt8(mColor.rgba[2]);
	writer.WriteUInt8(mColor.rgba[3]);
}

bool Swatch::operator==(const Swatch& other) const
{
	for (int i = 0; i < 4; ++i)
		if (mColor.rgba[i] != other.mColor.rgba[i])
			return false;
	
	return true;
}

// SwatchMgr

SwatchMgr::SwatchMgr()
{
	mSwatches = 0;
	mCursor = 0;
	mCount = 0;
	mMaxSwatches = 0;
}

SwatchMgr::~SwatchMgr()
{
	Capacity_set(0);
}

void SwatchMgr::Capacity_set(int maxSwatches)
{
	delete[] mSwatches;
	mSwatches = 0;
	mCursor = 0;
	mCount = 0;
	mMaxSwatches = 0;
	
	if (maxSwatches > 0)
	{
		mSwatches = new Swatch[maxSwatches];
		mMaxSwatches = maxSwatches;
	}
}

void SwatchMgr::Add(Swatch swatch)
{
	mSwatches[mCursor] = swatch;
	
	mCursor++;
	
	if (mCursor == mMaxSwatches)
		mCursor = 0;
	
	mCount++;
	
	if (mCount > mMaxSwatches)
		mCount = mMaxSwatches;
}

void SwatchMgr::AddOrUpdate(Swatch swatch)
{
	int index = FindIndex(swatch);
	
	if (index >= 0)
		Erase(index);
	
	Add(swatch);
}

void SwatchMgr::Erase(int index)
{
	for (int i = index + 1; i < mCount; ++i)
	{
		Swatch_set(i - 1, Swatch_get(i));
	}
	
	mCount--;
}

void SwatchMgr::Clear()
{
	mCount = 0;
}

int SwatchMgr::SwatchCount_get() const
{
	return mCount;
}

Swatch SwatchMgr::Swatch_get(int index) const
{
	return mSwatches[GetArrayIndex(index)];
}

void SwatchMgr::Swatch_set(int index, Swatch swatch)
{
	mSwatches[GetArrayIndex(index)] = swatch;
}

void SwatchMgr::Read(Stream* stream)
{
	StreamReader reader(stream, false);
	
	uint32_t count = reader.ReadUInt32();
	
	LOG_DBG("SwatchMgr: Read: count=%lu", count);
	
	Capacity_set(count);
	
	for (size_t i = 0; i < count; ++i)
	{
		Swatch swatch;
		
		swatch.Read(stream);
		
		Add(swatch);
	}
}

void SwatchMgr::Write(Stream* stream) const
{
	StreamWriter writer(stream, false);
	
	LOG_DBG("SwatchMgr: Write: count=%d", mCount);
	
	writer.WriteUInt32(mCount);
	
	for (int i = 0; i < mCount; ++i)
	{
		Swatch_get(i).Write(stream);
	}
}

int SwatchMgr::FindIndex(Swatch swatch) const
{
	for (int i = 0; i < mCount; ++i)
		if (Swatch_get(i) == swatch)
			return i;
	
	return -1;
}

int SwatchMgr::GetArrayIndex(int index) const
{
	Assert(index >= 0 && index <= mCount);
	
	index = mCursor - 1 - index;
	
	while (index < 0)
		index += mMaxSwatches;
	
	return index;
}

