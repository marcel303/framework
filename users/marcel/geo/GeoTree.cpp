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
	
	void Tree::Split(const Plane& plane, Tree* outTreeFront, Tree* outTreeBack, GenerateStatistics* generateStatistics)
	{

		Assert(outTreeFront);
		Assert(outTreeBack);

		for (std::list<Poly*>::iterator i = mesh.polys.begin(); i != mesh.polys.end(); ++i)
		{
		
			PlaneClassification classification = (*i)->Classify(plane);
			
			if (classification == pcOn)
			{
			
				Poly* poly = outTreeFront->mesh.Add();
				
				*poly = *(*i);
				
			}
			if (classification == pcFront)
			{
			
				Poly* poly = outTreeFront->mesh.Add();
				
				*poly = *(*i);
				
			}
			if (classification == pcBack)
			{
			
				Poly* poly = outTreeBack->mesh.Add();
				
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
				
				outTreeFront->mesh.Link(polyFront);
				outTreeBack->mesh.Link(polyBack);
				
			}
		
		}
		
		outTreeFront->Finalize();
		outTreeBack->Finalize();

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
			if (classification == pcFront)
			{
				++splitInfo.nFront;
			}
			if (classification == pcBack)
			{
				++splitInfo.nBack;
			}
			if (classification == pcSpan)
			{
				++splitInfo.nSpan;
			}
			
		}
		
		return splitInfo;

	}

};
