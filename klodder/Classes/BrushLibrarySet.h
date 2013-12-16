#pragma once

#include <vector>
#include "klodder_forward.h"

class BrushLibrarySet
{
public:
	BrushLibrarySet();
	
	Brush_Pattern* Find(uint32_t patternId);
	std::vector<Brush_Pattern*> BrushList_get();
	
	void Add(BrushLibrary* library, bool takeOwnership);
	
private:
	std::vector<BrushLibrary*> mLibraryList;
};
