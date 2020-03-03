#include "Debugging.h"
#include "GeoCsg3D.h"

// FIXME: Create function to safely unfinalize mesh.

namespace Geo
{

	TreeBsp* Csg3D::CreateBspTree(const Mesh& mesh)
	{

		TreeBsp* tree = new TreeBsp;
		tree->mesh = mesh;

		tree->Finalize();

		tree->Generate(100, 0);

		return tree;

	}

	void Csg3D::Filter(const TreeBsp& tree, const Mesh& mesh, Mesh& out_insideMesh, Mesh& out_outsideMesh)
	//void CCsg3D::clip(CMesh* mesh, CBsp* bsp, CMesh* out, CMesh* in, int polyClipPriority)
	{

		if (tree.children.size() == 0)
		{

			// Child leaf.

			for (std::list<Poly*>::const_iterator i = mesh.polys.begin(); i != mesh.polys.end(); ++i)
			{
			
				PlaneClassification classification = (*i)->Classify(tree.GetSplitPlane());
				
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
					
					Poly* poly = out_outsideMesh.Add();
					
					*poly = *(*i);

					poly->Finalize();
					
					#endif
				
					printf("On. Warning: 2D clipping not yet implemented.\n");
					
					#endif
				
				}
				
				if (classification == pcSpan)
				{

					// Spanning.. clip polygon into inside and outside.

					Poly* front = out_outsideMesh.Add();
					Poly* back = out_insideMesh.Add();
					
					(*i)->Split(tree.GetSplitPlane(), front, back);

					front->Finalize();
					back->Finalize();
					
					//printf("Span.\n");
					
				}
				
				if (classification == pcFront)
				{
				
					// Outside.
					
					Poly* poly = out_outsideMesh.Add();
					
					*poly = *(*i);

					poly->Finalize();
					
					//printf("Front.\n");
					
				}
				
				if (classification == pcBack)
				{
				
					// Inside.
					
					Poly* poly = out_insideMesh.Add();
					
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

			for (std::list<Poly*>::const_iterator i = mesh.polys.begin(); i != mesh.polys.end(); ++i)
			{

				PlaneClassification classification = (*i)->Classify(tree.GetSplitPlane());
				
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
					(*i)->Split(tree.GetSplitPlane(), front, back);
					front->Finalize();
					back->Finalize();
				}

			}

			// Pass the intermediate meshes down to the children.

			TreeBsp* child[2] =
			{
				(TreeBsp*)tree.children.front(),
				(TreeBsp*)tree.children.back()
			};
			
			for (int i = 0; i < 2; ++i)
			{
			
				if (tempMesh[i]->polys.empty() == false)
				{
					Filter(*child[i], *tempMesh[i], out_insideMesh, out_outsideMesh/*, polyClipPriority*/);
				}
				
				delete tempMesh[i];
				
			}

		}

	}

	void Csg3D::Add(const Mesh& mesh1, const Mesh& mesh2, Mesh& out_mesh)
	{

		TreeBsp* tree1 = CreateBspTree(mesh1);
		TreeBsp* tree2 = CreateBspTree(mesh2);

		Mesh* temp = new Mesh;
		
		Filter(*tree1, mesh2, *temp, out_mesh);
		Filter(*tree2, mesh1, *temp, out_mesh);
		
		delete temp;
		
		delete tree1;
		delete tree2;

	}

	void Csg3D::Subtract(const Mesh& mesh1, const Mesh& mesh2, Mesh& out_mesh)
	{

		TreeBsp* tree1 = CreateBspTree(mesh1);
		TreeBsp* tree2 = CreateBspTree(mesh2);
		
		Mesh temp1;
		Mesh temp2;
		
		Filter(*tree1, mesh2, temp1, temp2); // Inside.
		Filter(*tree2, mesh1, temp2, out_mesh); // Outside.

		for (std::list<Poly*>::iterator i = temp1.polys.begin(); i != temp1.polys.end(); ++i)
		{
			if ((*i)->bFinalized)
			{
				(*i)->Unfinalize();
			}
		}
		
		temp1.RevertPolyWinding();
		temp1.Move(out_mesh);
		
		for (std::list<Poly*>::iterator i = out_mesh.polys.begin(); i != out_mesh.polys.end(); ++i)
		{
			if ((*i)->bFinalized)
			{
				(*i)->Unfinalize();
			}
		}
		
		delete tree1;
		delete tree2;

	}

	void Csg3D::Intersect(const Mesh& mesh1, const Mesh& mesh2, Mesh& out_mesh)
	{

		TreeBsp* tree1 = CreateBspTree(mesh2);
		
		Mesh temp;
		
		Filter(*tree1, mesh1, out_mesh, temp);
		
		delete tree1;

	}

	void Csg3D::AddInplace(Mesh& mesh1, const Mesh& mesh2)
	{
	
		Assert(false);
		
	}

	void Csg3D::SubtractInplace(Mesh& mesh1, const Mesh& mesh2)
	{

		Mesh out_mesh;
		
		Subtract(mesh1, mesh2, out_mesh);
		
		mesh1.Unfinalize();
		mesh1.Clear();
		out_mesh.Move(mesh1);
		mesh1.Finalize();

	}

	void Csg3D::IntersectInplace(Mesh& mesh1, const Mesh& mesh2)
	{
	
		Assert(false);
		
	}

} // Geo.
