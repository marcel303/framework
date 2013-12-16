#pragma once

#include "klodder_forward.h"
#include "MacImage.h"

class Swatch
{
public:
	Swatch();
	
	MacRgba Color_get() const;
	void Color_set(MacRgba rgba);
	
	void Read(Stream* stream);
	void Write(Stream* stream) const;
	
	bool operator==(const Swatch& other) const;
	
private:
	MacRgba mColor;
};

class SwatchMgr
{
public:
	SwatchMgr();
	~SwatchMgr();
	
	void Capacity_set(int maxSwatches);
	
	void Add(Swatch swatch);
	void AddOrUpdate(Swatch swatch);
	void Erase(int index);
	void Clear();
	
	int SwatchCount_get() const;
	Swatch Swatch_get(int index) const;
	void Swatch_set(int index, Swatch swatch);
	
	void Read(Stream* stream);
	void Write(Stream* stream) const;
	
private:
	int FindIndex(Swatch swatch) const;
	int GetArrayIndex(int index) const;
	
	Swatch* mSwatches;
	int mCursor;
	int mCount;
	int mMaxSwatches;
};
