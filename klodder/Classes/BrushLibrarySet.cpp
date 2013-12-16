#include "Brush_Pattern.h"
#include "BrushLibrary.h"
#include "BrushLibrarySet.h"
#include "Debugging.h"

BrushLibrarySet::BrushLibrarySet()
{
}

Brush_Pattern* BrushLibrarySet::Find(uint32_t patternId)
{
	for (size_t i = 0; i < mLibraryList.size(); ++i)
	{
		Brush_Pattern* pattern = mLibraryList[i]->Find(patternId);
		
		if (pattern)
			return pattern;
	}
	
	return 0;
}

std::vector<Brush_Pattern*> BrushLibrarySet::BrushList_get()
{
	std::vector<Brush_Pattern*> result;
	
	for (size_t i = 0; i < mLibraryList.size(); ++i)
	{
		for (size_t j = 0; j < mLibraryList[i]->BrushList_get().size(); ++j)
			result.push_back(mLibraryList[i]->BrushList_get()[j]);
	}
	
	return result;
}

void BrushLibrarySet::Add(BrushLibrary* library, bool takeOwnership)
{
	Assert(takeOwnership == false);
	
	mLibraryList.push_back(library);
}
