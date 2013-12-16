#pragma once

#include <vector>
#include "klodder_forward.h"
#include "libgg_forward.h"
#include "Types.h"

class BrushLibrary
{
public:
	~BrushLibrary();

	void Clear();

	void Load(Stream* stream, bool loadData);
	void Save(Stream* stream);

	void Append(Brush_Pattern* brush, bool takeOwnership);
	void AppendChunk(Stream* stream, Brush_Pattern* brush);

	Brush_Pattern* Find(uint32_t patternId);
	
	const std::vector<Brush_Pattern*>& BrushList_get() const
	{
		return mBrushList;
	}

private:
	std::vector<Brush_Pattern*> mBrushList;
};
