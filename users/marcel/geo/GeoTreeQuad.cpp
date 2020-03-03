#include "GeoTreeQuad.h"

namespace Geo
{

	TreeQuad::TreeQuad()
	{

		splitAxis = 0;
		splitTreshold = 2;
		
	}

	TreeQuad::~TreeQuad()
	{

	}

	void TreeQuad::SetSplitAxis(int axis)
	{

		splitAxis = axis;
		
	}

	void TreeQuad::SetSplitTreshold(int treshold)
	{

		splitTreshold = treshold;
		
	}

	void TreeQuad::Generate(int depthLeft, GenerateStatistics* generateStatistics)
	{

		if (depthLeft == 0)
		{
			return;
		}
		
		if (mesh.polys.size() < splitTreshold)
		{
			return;
		}
		
		splitPosition = mesh.GetPolyCenter();
		
		splitPosition[splitAxis] = 0.0f;
		
		Plane plane[2];
		
		plane[0].normal[(splitAxis + 0) % 3] = 0.0f;
		plane[0].normal[(splitAxis + 1) % 3] = 1.0f;
		plane[0].normal[(splitAxis + 2) % 3] = 0.0f;
		plane[0].distance = splitPosition * plane[0].normal;
		
		plane[1].normal[(splitAxis + 0) % 3] = 0.0f;
		plane[1].normal[(splitAxis + 1) % 3] = 0.0f;
		plane[1].normal[(splitAxis + 2) % 3] = 1.0f;
		plane[1].distance = splitPosition * plane[1].normal;
		
		#if 0
		
		printf("Split: %f %f %f - %f.\n",
			plane[0].normal[0],
			plane[0].normal[1],
			plane[0].normal[2],
			plane[0].distance);
			
		printf("Split: %f %f %f - %f.\n",
			plane[1].normal[0],
			plane[1].normal[1],
			plane[1].normal[2],
			plane[1].distance);
		
		#endif
		
		Tree* temp[2] = 
		{
			new Tree,
			new Tree
		};
		
		Split(plane[0], temp[0], temp[1], generateStatistics);

		mesh.Unfinalize();
		mesh.Clear();
		
		TreeQuad* child[4] =
		{
			new TreeQuad,
			new TreeQuad,
			new TreeQuad,
			new TreeQuad
		};
		
		temp[0]->Split(plane[1], child[0], child[1], generateStatistics);
		temp[1]->Split(plane[1], child[2], child[3], generateStatistics);
		
		delete temp[0];
		delete temp[1];
		
		#if 0
		
		printf("%d -> %d %d %d %d.\n",
			mesh->polys.size(),
			child[0]->mesh->polys.size(),
			child[1]->mesh->polys.size(),
			child[2]->mesh->polys.size(),
			child[3]->mesh->polys.size());
		
		#endif
		
		for (int i = 0; i < 4; ++i)
		{
			if (child[i]->mesh.polys.empty())
			{
				delete child[i];
				child[i] = nullptr;
			}
		}

		for (int i = 0; i < 4; ++i)
		{
			if (child[i])
			{
				child[i]->SetSplitAxis(splitAxis);
				child[i]->SetSplitTreshold(splitTreshold);
				child[i]->Generate(depthLeft - 1, generateStatistics);
				Link(child[i]);
			}
		}
		
	}

};
