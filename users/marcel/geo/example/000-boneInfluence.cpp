#include "Geo.h"
#include <math.h>
#include <stdlib.h> // abs, rand
#include <stdio.h> // printf

static void CreateGrid(int axis, Vec3Arg origin, Geo::Mesh* mesh);

int main(int argc, char* argv[])
{

	// Create bsp tree.
	
	Geo::TreeBsp* treeBsp = new Geo::TreeBsp();
	
	for (int i = 0; i < 100; ++i)
	{
	
		Geo::Poly* poly = treeBsp->mesh.Add();
		
		Vec3 position;
		
		for (int j = 0; j < 3; ++j)
		{
			position[j] = (((rand() & 4095) / 4095.0f) - 0.5f) * 100.0f;
		}
		
		for (int j = 0; j < 3; ++j)
		{
		
			Geo::Vertex* vertex = poly->Add();
			
			for (int k = 0; k < 3; ++k)
			{
				vertex->position[k] = position[k] + (((rand() & 4095) / 4095.0f) - 0.5f) * 2.0f;
			}
			
		}
		
	}
	
	// Finalize.
	
	treeBsp->mesh.Finalize();
	
	// Split bsp tree.
	
	Geo::Tree::GenerateStatistics generateStatistics;
	
	treeBsp->Generate(100, &generateStatistics);
	
	printf("Tree: nNodes: %d.\n", generateStatistics.nNodes);
	printf("Tree: nLeafs: %d.\n", generateStatistics.nLeafs);
	printf("Tree: nSplits: %d.\n", generateStatistics.nSplits);
	printf("Tree: nPoly: %d.\n", generateStatistics.nPoly);
	printf("Tree: nVertex: %d.\n", generateStatistics.nVertex);
	
	delete treeBsp;
	treeBsp = nullptr;
	
	// Test bone structure.
	
	Geo::Bone* bone1 = new Geo::Bone;
	
	Geo::Bone* bone1_1 = new Geo::Bone;
	
	{
		Geo::BoneInfluenceCylinderCapped* boneInfluence = new Geo::BoneInfluenceCylinderCapped;
		boneInfluence->m_radius = 0.1f;
		bone1_1->influences.push_back(boneInfluence);
	}
	
	bone1->children.push_back(bone1_1);
	
	bone1->Finalize();
	
	//bone1_1->m_position = Vector(-1.0f, 0.0f, 0.0f);
	bone1_1->currentRotationLocal.MakeRotationZ(float(M_PI) / 2.0f);
	
	for (std::list<Geo::Bone*>::iterator i = bone1->children.begin(); i != bone1->children.end(); ++i)
	{
	
		for (float x = -2.0f; x <= +2.0f; x += 0.1f)
		{
		
			Vec3 position(x, 0.0f, 0.0f);
			
			float influence = (*i)->CalculateInfluence(position);
			
			printf("Influence @ %+04.2f): %+04.2f.\n", x, influence);
			
		}
	
	}
	
	Geo::Mesh* mesh = new Geo::Mesh;
	
	CreateGrid(0, Vec3(-1.0f, 0.0f, 0.0f), mesh);
	CreateGrid(0, Vec3(+1.0f, 0.0f, 0.0f), mesh);
	CreateGrid(1, Vec3(0.0f, -1.0f, 0.0f), mesh);
	CreateGrid(1, Vec3(0.0f, +1.0f, 0.0f), mesh);
	CreateGrid(2, Vec3(0.0f, 0.0f, -1.0f), mesh);
	CreateGrid(2, Vec3(0.0f, 0.0f, +1.0f), mesh);
	
	mesh->Finalize();
	
	printf("Mesh: nPoly: %d.\n", (int)mesh->polys.size());
	
	for (std::list<Geo::Poly*>::iterator i = mesh->polys.begin(); i != mesh->polys.end(); ++i)
	{
	
		for (std::list<Geo::Vertex*>::iterator j = (*i)->vertices.begin(); j != (*i)->vertices.end(); ++j)
		{
		
			Vec3 delta = (*j)->position;
			delta.Normalize();
			
			Vec3 size;
			
			for (int k = 0; k < 3; ++k)
			{
				size[k] = abs(delta[k]);
			}
			
			//(*j)->position = delta ^ size;
			
			for (std::list<Geo::Bone*>::iterator k = bone1->children.begin(); k != bone1->children.end(); ++k)
			{
	
				float influence = (*k)->CalculateInfluence((*j)->position);
				
				printf("Influence: %+04.2ff.\n", influence);
				
			}
			
		}
	}

	delete mesh;
	mesh = nullptr;
	
	delete bone1;
	bone1 = nullptr;
	
	return 0;
	
}

static void CreateGrid(int axis, Vec3Arg origin, Geo::Mesh* mesh)
{

	int subDiv1 = 5;
	int subDiv2 = 5;
	
	for (int i1 = 0; i1 < subDiv1; ++i1)
	{
	
		for (int i2 = 0; i2 < subDiv2; ++i2)
		{
		
			float v11 = (i1 + 0) - subDiv1 / 2.0f;
			float v12 = (i2 + 0) - subDiv2 / 2.0f;

			float v21 = (i1 + 1) - subDiv1 / 2.0f;
			float v22 = (i2 + 0) - subDiv2 / 2.0f;
			
			float v31 = (i1 + 1) - subDiv1 / 2.0f;
			float v32 = (i2 + 1) - subDiv2 / 2.0f;
			
			float v41 = (i1 + 0) - subDiv1 / 2.0f;
			float v42 = (i2 + 1) - subDiv2 / 2.0f;		
			
			Geo::Poly* poly = mesh->Add();
			
			Geo::Vertex* vertex1 = poly->Add();
			Geo::Vertex* vertex2 = poly->Add();
			Geo::Vertex* vertex3 = poly->Add();
			Geo::Vertex* vertex4 = poly->Add();
			
			vertex1->position[(axis + 1) % 3] = v11;
			vertex1->position[(axis + 2) % 3] = v12;
			
			vertex2->position[(axis + 1) % 3] = v21;
			vertex2->position[(axis + 2) % 3] = v22;

			vertex3->position[(axis + 1) % 3] = v31;
			vertex3->position[(axis + 2) % 3] = v32;

			vertex4->position[(axis + 1) % 3] = v41;
			vertex4->position[(axis + 2) % 3] = v42;
			
			vertex1->position += origin;
			vertex2->position += origin;
			vertex3->position += origin;
			vertex4->position += origin;
		
		}
		
	}

}
