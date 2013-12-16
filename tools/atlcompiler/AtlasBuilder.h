#pragma once

#include <algorithm>
#include <vector>
#include "AtlasNode.h"

class AtlasBuilder
{
public:
	AtlasBuilder()
	{
	}

	AtlasNode* Create(std::vector<Atlas_ImageInfo*> images)
	{
		std::sort(images.begin(), images.end(), Atlas_ImageInfo::CompareBySize);

		//

		AtlasNode* root = new AtlasNode(0);
		root->m_BB.m_Position = PointI(0, 0);
		root->m_BB.m_Size = PointI(1024, 1024);

		root->Setup(7);

		printf("LeafCount: %d\n", root->m_LeafCount);

		for (size_t i = 0; i < images.size(); ++i)
		{
			Atlas_ImageInfo* image = images[i];

			AtlasNode* node = root->FindSpace(image->m_Size);

			if (node)
			{
				node->Allocate();

				node->m_Image = image;
			}
			else
			{
				printf("unable to allocate node\n");
			}
		}

		return root;
	}
};
