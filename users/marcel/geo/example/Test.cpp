#include <vld.h>
#include "Geo.h"
#include "Math.h"

static void CreateGrid(int axis, Vector origin, Geo::Mesh* mesh);

int main(int argc, char* argv[])
{

	// Create bsp tree.
	
	Geo::TreeBsp* treeBsp = new Geo::TreeBsp;
	
	for (int i = 0; i < 100; ++i)
	{
	
		Geo::Poly* poly = treeBsp->mesh->Add();
		
		Vector position;
		
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
	
	treeBsp->mesh->Finalize();
	
	// Split bsp tree.
	
	Geo::Tree::GenerateStatistics generateStatistics;
	
	//treeBsp->Generate(100, &generateStatistics);
	
	printf("Tree: nNodes: %d.\n", generateStatistics.nNodes);
	printf("Tree: nLeafs: %d.\n", generateStatistics.nLeafs);
	printf("Tree: nSplits: %d.\n", generateStatistics.nSplits);
	printf("Tree: nPoly: %d.\n", generateStatistics.nPoly);
	printf("Tree: nVertex: %d.\n", generateStatistics.nVertex);
	
	delete treeBsp;
	
	// Test bone structure.
	
	Geo::Bone* bone1 = new Geo::Bone;
	
	Geo::Bone* bone1_1 = new Geo::Bone;
	
	{
		Geo::BoneInfluenceCilinderCapped* boneInfluence = new Geo::BoneInfluenceCilinderCapped;
		boneInfluence->m_radius = 0.1f;
		bone1_1->cInfluence.push_back(boneInfluence);
	}
	
	bone1->cChild.push_back(bone1_1);
	
	//bone1_1->m_position = Vector(-1.0f, 0.0f, 0.0f);
	bone1_1->m_rotation.MakeRotationEuler(Vector(0.0f, 0.0f, M_PI / 2.0f));
	
	for (std::list<Geo::Bone*>::iterator i = bone1->cChild.begin(); i != bone1->cChild.end(); ++i)
	{
	
		for (float x = -2.0f; x <= +2.0f; x += 0.1f)
		{
			Vector position(x, 0.0f, 0.0f);
			float influence = (*i)->CalculateInfluence(position);
			printf("Influence @ %+04.2f): %+04.2ff.\n", x, influence);
		}
	
	}
	
	Geo::Mesh* mesh = new Geo::Mesh;
	
	CreateGrid(0, Vector(-1.0f, 0.0f, 0.0f), mesh);
	CreateGrid(0, Vector(+1.0f, 0.0f, 0.0f), mesh);
	CreateGrid(1, Vector(0.0f, -1.0f, 0.0f), mesh);
	CreateGrid(1, Vector(0.0f, +1.0f, 0.0f), mesh);
	CreateGrid(2, Vector(0.0f, 0.0f, -1.0f), mesh);
	CreateGrid(2, Vector(0.0f, 0.0f, +1.0f), mesh);
	
	printf("Mesh: nPoly: %d.\n", mesh->cPoly.size());
	
	for (std::list<Geo::Poly*>::iterator i = mesh->cPoly.begin(); i != mesh->cPoly.end(); ++i)
	{
		for (std::list<Geo::Vertex*>::iterator j = (*i)->cVertex.begin(); j != (*i)->cVertex.end(); ++j)
		{
			Vector delta = (*j)->position;
			delta.Normalize();
			Vector size;
			for (int k = 0; k < 3; ++k)
			{
				size[k] = abs(delta[k]);
			}
			//(*j)->position = delta ^ size;
			
			for (std::list<Geo::Bone*>::iterator k = bone1->cChild.begin(); k != bone1->cChild.end(); ++k)
			{
	
				float influence = (*k)->CalculateInfluence((*j)->position);
				printf("Influence: %+04.2ff.\n", influence);
				
			}
			
		}
	}
	
	mesh->Finalize();

	delete mesh;
	
	delete bone1;
	
	return 0;
	
}

static void CreateGrid(int axis, Vector origin, Geo::Mesh* mesh)
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
	
	mesh->Finalize();

}