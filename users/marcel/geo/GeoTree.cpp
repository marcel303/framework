#include "Debugging.h"
#include "GeoTree.h"

namespace Geo
{

	Tree::Tree()
	{
		
	}

	Tree::~Tree()
	{

		Clear();

	}

	Tree* Tree::Link(Tree* child)
	{

		Assert(child);

		children.push_back(child);

		return child;
		
	};

	void Tree::Remove(Tree* child)
	{

		Assert(child);
		
		for (std::list<Tree*>::iterator i = children.begin(); i != children.end(); ++i)
		{
		
			if ((*i) == child)
			{
				delete (*i);
				children.erase(i);
				return;
			}
			
		}
		
	}

	void Tree::Clear()
	{

		while (children.empty() == false)
		{
			Remove(children.front());
		}
		
	}

	void Tree::Finalize()
	{

		mesh.Finalize();
		
		aabb = mesh.GetExtents();
		
	}
	
	void Tree::Split(const Plane& plane, Tree* out_treeFront, Tree* out_treeBack, GenerateStatistics* generateStatistics)
	{

		Assert(out_treeFront != nullptr);
		Assert(out_treeBack != nullptr);

		for (std::list<Poly*>::iterator i = mesh.polys.begin(); i != mesh.polys.end(); ++i)
		{
		
			PlaneClassification classification = (*i)->Classify(plane);
			
			if (classification == pcOn)
			{
			
				Poly* poly = out_treeFront->mesh.Add();
				
				*poly = *(*i);
				
			}
			if (classification == pcFront)
			{
			
				Poly* poly = out_treeFront->mesh.Add();
				
				*poly = *(*i);
				
			}
			if (classification == pcBack)
			{
			
				Poly* poly = out_treeBack->mesh.Add();
				
				*poly = *(*i);
			
			}
			if (classification == pcSpan)
			{
			
				if (generateStatistics)
				{
					++generateStatistics->nSplits;
				}
				
				Poly* polyFront = new Poly;
				Poly* polyBack = new Poly;
				
				(*i)->Split(plane, polyFront, polyBack);
				
				out_treeFront->mesh.Link(polyFront);
				out_treeBack->mesh.Link(polyBack);
				
			}
		
		}
		
		out_treeFront->Finalize();
		out_treeBack->Finalize();

	}

	Tree::GenerateStatistics::GenerateStatistics()
	{

		nLeafs = 0;
		nNodes = 0;
		nSplits = 0;
		nPoly = 0;
		nVertex = 0;

	}
	
	void Tree::Generate(int depthLeft, GenerateStatistics* generateStatistics)
	{

		Assert(false);
		
	}

	Tree::SplitInfo Tree::GetSplitInfo(const Plane& plane) const
	{

		SplitInfo splitInfo;
		
		for (std::list<Poly*>::const_iterator i = mesh.polys.begin(); i != mesh.polys.end(); ++i)
		{
		
			PlaneClassification classification = (*i)->Classify(plane);
			
			if (classification == pcOn)
			{
				++splitInfo.nOn;
			}
			else if (classification == pcFront)
			{
				++splitInfo.nFront;
			}
			else if (classification == pcBack)
			{
				++splitInfo.nBack;
			}
			else if (classification == pcSpan)
			{
				++splitInfo.nSpan;
			}
			
		}
		
		return splitInfo;

	}

}
