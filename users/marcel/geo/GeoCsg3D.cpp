#include "Debugging.h"
#include "GeoCsg3D.h"

// FIXME: Create function to safely unfinalize mesh.

namespace Geo
{

namespace Csg3D
{

TreeBsp* CreateBspTree(Mesh* mesh)
{

	TreeBsp* tree = new TreeBsp;
	*tree->mesh = *mesh;

	tree->Finalize();

	tree->Generate(100, 0);

	return tree;

}

void Filter(TreeBsp* tree, Mesh* mesh, Mesh* outInside, Mesh* outOutside)
//void CCsg3D::clip(CMesh* mesh, CBsp* bsp, CMesh* out, CMesh* in, int polyClipPriority)
{

	Assert(tree);
	Assert(mesh);
	Assert(outInside);
	Assert(outOutside);
	
	if (tree->cChild.size() == 0)
	{

		// Child leaf.

		for (std::list<Poly*>::iterator i = mesh->cPoly.begin(); i != mesh->cPoly.end(); ++i)
		{
		
			PlaneClassification classification = (*i)->Classify(tree->GetSplitPlane());
			
			if (classification == pcOn)
			{

				// Coplanar, do a 2D CSG with the polygon in the BSP leaf.

				#if 0
				
				if (tree->cChild.size() > 0)
				{
     				if (!polyClipPriority)
					{
						Poly* inTemp = new Poly;
						(*i)->Clip(tree->cChild.front(), inTemp, outOutside);
						if (inTemp->cVertex.size() >= 3)
						{
							inTemp->Finalize();
							outInside->Link(inTemp);
						{
						else
						{
							delete inTemp;
						}
					}
					else
					{
						Poly* poly = outOutside->Add()
						*poly = *(*i);
						poly->Finalize();
					}
				}
				
				#else
			
				// Just add polygon. This will cause overlapping polygons & possibly z-fighting.
				
				#if 1
				
				Poly* poly = outOutside->Add();
				
				*poly = *(*i);

				poly->Finalize();
				
				#endif
			
				printf("On. Warning: 2D clipping not yet implemented.\n");
				
				#endif
			
			}
			
			if (classification == pcSpan)
			{

				// Spanning.. clip polygon into inside and outside.

				Poly* front = outOutside->Add();
				Poly* back = outInside->Add();
				
				(*i)->Split(tree->GetSplitPlane(), front, back);

				front->Finalize();
				back->Finalize();
				
				//printf("Span.\n");
				
			}
			
			if (classification == pcFront)
			{
			
				// Outside.
				
				Poly* poly = outOutside->Add();
				
				*poly = *(*i);

				poly->Finalize();
				
				//printf("Front.\n");
				
			}
			
			if (classification == pcBack)
			{
			
				// Inside.
				
				Poly* poly = outInside->Add();
				
				*poly = *(*i);

				poly->Finalize();
				
				//printf("Back.\n");
				
			}

		}

	}
	else
	{

		// Clip all polygons and send them to front or back intermediate meshes..

		Mesh* tempMesh[2];

		tempMesh[0] = new Mesh;
		tempMesh[1] = new Mesh;

		for (std::list<Poly*>::iterator i = mesh->cPoly.begin(); i != mesh->cPoly.end(); ++i)
		{

			PlaneClassification classification = (*i)->Classify(tree->GetSplitPlane());
			
			if (classification == pcOn || classification == pcFront)
			{
				Poly* poly = tempMesh[0]->Add();
				*poly = *(*i);
				poly->Finalize();
			}
			if (classification == pcBack)
			{
				Poly* poly = tempMesh[1]->Add();
				*poly = *(*i);
				poly->Finalize();
			}
			if (classification == pcSpan)
			{
				Poly* front = tempMesh[0]->Add();
				Poly* back = tempMesh[1]->Add();
				(*i)->Split(tree->GetSplitPlane(), front, back);
				front->Finalize();
				back->Finalize();
			}

		}

		// Pass the intermediate meshes down to the children.

		TreeBsp* child[2] =
		{
			(TreeBsp*)tree->cChild.front(),
			(TreeBsp*)tree->cChild.back()
		};
		
		for (int i = 0; i < 2; ++i)
		{
		
			if (tempMesh[i]->cPoly.size() > 0)
			{
				Filter(child[i], tempMesh[i], outInside, outOutside/*, polyClipPriority*/);
			}
				
			delete tempMesh[i];
			
		}

	}

}

void Add(Mesh* mesh1, Mesh* mesh2, Mesh* outMesh)
{

	TreeBsp* tree1 = CreateBspTree(mesh1);
	TreeBsp* tree2 = CreateBspTree(mesh2);

	Mesh* temp = new Mesh;
		
	Filter(tree1, mesh2, temp, outMesh);
	Filter(tree2, mesh1, temp, outMesh);
	
	delete temp;
		
	delete tree1;
	delete tree2;

}

void Subtract(Mesh* mesh1, Mesh* mesh2, Mesh* outMesh)
{

	TreeBsp* tree1 = CreateBspTree(mesh1);
	TreeBsp* tree2 = CreateBspTree(mesh2);
	
	Mesh* temp1 = new Mesh;
	Mesh* temp2 = new Mesh;
	
	Filter(tree1, mesh2, temp1, temp2); // Inside.
	Filter(tree2, mesh1, temp2, outMesh); // Outside.

	for (std::list<Poly*>::iterator i = temp1->cPoly.begin(); i != temp1->cPoly.end(); ++i)
	{
		if ((*i)->bFinalized)
		{
			(*i)->Unfinalize();
		}
	}
		
	temp1->RevertPolyWinding();
	temp1->Move(outMesh);
	
	for (std::list<Poly*>::iterator i = outMesh->cPoly.begin(); i != outMesh->cPoly.end(); ++i)
	{
		if ((*i)->bFinalized)
		{
			(*i)->Unfinalize();
		}
	}
	
	delete temp1;
	delete temp2;
	
	delete tree1;
	delete tree2;

}

void Intersect(Mesh* mesh1, Mesh* mesh2, Mesh* outMesh)
{

	TreeBsp* tree1 = CreateBspTree(mesh2);
	
	Mesh* temp = new Mesh;
	
	Filter(tree1, mesh1, outMesh, temp);
	
	delete temp;
	
	delete tree1;

}

void AddInplace(Mesh* mesh1, Mesh* mesh2)
{

}

void SubtractInplace(Mesh* mesh1, Mesh* mesh2)
{

	Mesh* outMesh = new Mesh;
	
	Subtract(mesh1, mesh2, outMesh);
	
	mesh1->Unfinalize();
	mesh1->Clear();
	outMesh->Move(mesh1);
	delete outMesh;
	mesh1->Finalize();

}

void IntersectInplace(Mesh* mesh1, Mesh* mesh2)
{

}

}; // Csg3D.

}; // Geo.
