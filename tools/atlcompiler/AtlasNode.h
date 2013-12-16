#pragma once

#include "Atlas_ImageInfo.h"
#include "Types.h"

class AtlasNode
{
public:
	AtlasNode(AtlasNode* parent);
	void Setup(int d);

	void UpdateDsc();
	void UpdateAsc();

	void Allocate();
	AtlasNode* FindSpace(PointI size);

	bool IsLeaf_get() const;

	Atlas_ImageInfo* m_Image;
	AtlasNode* m_Parent;
	AtlasNode* m_Children[4];
	
	RectI m_BB;
	int m_LeafCount;
	int m_UsedLeafCount;
};
