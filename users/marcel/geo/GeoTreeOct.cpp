#include "GeoTreeOct.h"

namespace Geo
{

TreeOct::TreeOct()
{

	splitTreshold = 2;
	
}

TreeOct::~TreeOct()
{

}

void TreeOct::SetSplitTreshold(int treshold)
{

	splitTreshold = treshold;
	
}

void TreeOct::Generate(int depthLeft, GenerateStatistics* generateStatistics)
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
	
	Plane plane[3];
	
	plane[0].normal[0] = 1.0f;
	plane[0].normal[1] = 0.0f;
	plane[0].normal[2] = 0.0f;
	plane[0].distance = splitPosition * plane[0].normal;
	
	plane[1].normal[0] = 0.0f;
	plane[1].normal[1] = 1.0f;
	plane[1].normal[2] = 0.0f;
	plane[1].distance = splitPosition * plane[1].normal;
	
	plane[2].normal[0] = 0.0f;
	plane[2].normal[1] = 0.0f;
	plane[2].normal[2] = 1.0f;
	plane[2].distance = splitPosition * plane[2].normal;
	
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
	
	Tree* temp1[2] = 
	{
		new Tree,
		new Tree
	};
	
	Split(plane[0], temp1[0], temp1[1], generateStatistics);
	
	mesh.Unfinalize();
	mesh.Clear();
	
	Tree* temp2[4] =
	{
		new Tree,
		new Tree,
		new Tree,
		new Tree
	};
	
	temp1[0]->Split(plane[1], temp2[0], temp2[1], generateStatistics);
	temp1[1]->Split(plane[1], temp2[2], temp2[3], generateStatistics);
	
	delete temp1[0];
	delete temp1[1];
	
	TreeOct* child[8] =
	{
		new TreeOct,
		new TreeOct,
		new TreeOct,
		new TreeOct,
		new TreeOct,
		new TreeOct,
		new TreeOct,
		new TreeOct
	};
	
	temp2[0]->Split(plane[2], child[0], child[1], generateStatistics);
	temp2[1]->Split(plane[2], child[2], child[3], generateStatistics);
	temp2[2]->Split(plane[2], child[4], child[5], generateStatistics);
	temp2[3]->Split(plane[2], child[6], child[7], generateStatistics);
	
	delete temp2[0];
	delete temp2[1];
	delete temp2[2];
	delete temp2[3];
	
	#if 0
	
	printf("%d -> %d %d %d %d %d %d %d %d.\n",
		mesh->polys.size(),
		child[0]->mesh->polys.size(),
		child[1]->mesh->polys.size(),
		child[2]->mesh->polys.size(),
		child[3]->mesh->polys.size(),
		child[4]->mesh->polys.size(),
		child[5]->mesh->polys.size(),
		child[6]->mesh->polys.size(),
		child[7]->mesh->polys.size());
	
	#endif
	
	for (int i = 0; i < 8; ++i)
	{
		if (child[i]->mesh.polys.empty())
		{
			delete child[i];
			child[i] = nullptr;
		}
	}
	
	for (int i = 0; i < 8; ++i)
	{
		if (child[i])
		{
			child[i]->SetSplitTreshold(splitTreshold);
			child[i]->Generate(depthLeft - 1, generateStatistics);
			Link(child[i]);
		}
	}
	
}

};
