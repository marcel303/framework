#include "Debugging.h"
#include "GeoTreeKd.h"

namespace Geo
{

	TreeKd::TreeKd() : Tree()
	{

		splitAxis = 0;
		splitTreshold = 2;
		
	}

	TreeKd::~TreeKd()
	{

	}

	void TreeKd::Generate(int depthLeft, GenerateStatistics* generateStatistics)
	{

		if (depthLeft == 0)
		{
			return;
		}
		
		aabb = mesh.CalculateExtents();
		
		splitAxis = CalculateSplitAxis();
		
		Plane plane;
		
		if (CalculateSplitPlane(&plane))
		{
		
			if (generateStatistics)
			{
				generateStatistics->nNodes += 2;
			}
			
			TreeKd* childFront = new TreeKd;
			TreeKd* childBack = new TreeKd;
			
			Tree::Split(plane, childFront, childBack, generateStatistics);
			
			mesh.Clear();
			
			splitPlane = plane;
			
			Link(childFront);
			Link(childBack);
			
			childFront->Generate(depthLeft - 1, generateStatistics);
			childBack->Generate(depthLeft - 1, generateStatistics);
			
		}
		else
		{
		
			if (generateStatistics)
			{
			
				++generateStatistics->nLeafs;
			
				generateStatistics->nPoly += (int)mesh.polys.size();
				
				for (std::list<Poly*>::const_iterator i = mesh.polys.begin(); i != mesh.polys.end(); ++i)
				{
					generateStatistics->nVertex += (int)(*i)->vertices.size();
				}
				
			}
		
		}

	}

	int TreeKd::CalculateSplitAxis() const
	{

		Vec3 aabbSize = aabb.max - aabb.min;
		
		int axis;
		axis = aabbSize[0] > aabbSize[1] ? 0 : 1;
		axis = aabbSize[2] > aabbSize[axis] ? 2 : axis;

		return axis;
		
	}

	void TreeKd::CalculateSplitVector(std::vector<KdSplitInfo>& vSplitInfo) const
	{

		for (std::list<Poly*>::const_iterator i = mesh.polys.begin(); i != mesh.polys.end(); ++i)
		{
		
			Vec3 aabbMin;
			Vec3 aabbMax;
			
			(*i)->CalculateExtents(aabbMin, aabbMax);
			
			KdSplitInfo splitInfo1;
			KdSplitInfo splitInfo2;
			
			splitInfo1.d = aabbMin[splitAxis] - Geo::epsilon;
			splitInfo2.d = aabbMax[splitAxis] + Geo::epsilon;
			
			vSplitInfo.push_back(splitInfo1);
			vSplitInfo.push_back(splitInfo2);
			
		}

	}


	void TreeKd::CalculateSplitCosts(std::vector<KdSplitInfo>& vSplitInfo) const
	{

		// Perform surface area hueristic.
		
		Aabb aabbInfinite;
		
		for (int i = 0; i < 3; ++i)
		{
			aabbInfinite.min[i] = -1000000000.0f;
			aabbInfinite.max[i] = +1000000000.0f;
		}

		Aabb aabbFront = aabbInfinite;
		Aabb aabbBack = aabbInfinite;

		for (std::vector<KdSplitInfo>::iterator i = vSplitInfo.begin(); i != vSplitInfo.end(); ++i)
		{

			aabbFront.min[splitAxis] = i->d;
			aabbBack.max[splitAxis] = i->d;

			std::vector<Poly*> vPolyFront;
			std::vector<Poly*> vPolyBack;
			
			for (std::list<Poly*>::const_iterator j = mesh.polys.begin(); j != mesh.polys.end(); ++j)
			{
			
				Aabb aabb;
				
				(*j)->CalculateExtents(aabb.min, aabb.max);
				
				if (aabbFront.IsIntersecting(aabb))
				{
					vPolyFront.push_back((*j));
				}
				if (aabbBack.IsIntersecting(aabb))
				{
					vPolyBack.push_back((*j));
				}
				
			}
			
			i->nFront = (int)vPolyFront.size();
			i->nBack = (int)vPolyBack.size();
			
			// Calculate bounding boxes.
			
			Aabb aabbFinalFront;
			Aabb aabbFinalBack;
			
			for (int j = 0; j < vPolyFront.size(); ++j)
			{
				Aabb aabb;
				vPolyFront[j]->CalculateExtents(aabb.min, aabb.max);
				aabbFinalFront.Add(aabb);
			}
			
			for (int j = 0; j < vPolyBack.size(); ++j)
			{
				Aabb aabb;
				vPolyBack[j]->CalculateExtents(aabb.min, aabb.max);
				aabbFinalBack.Add(aabb);
			}
			
			// Calculate costs.
			
			Vec3 sizeFront = aabbFinalFront.CalculateSize();
			Vec3 sizeBack = aabbFinalBack.CalculateSize();

			const float costFront = 2.0f * (sizeFront[0] * sizeFront[2] + sizeFront[1] * sizeFront[0] + sizeFront[2] * sizeFront[1]);
			const float costBack = 2.0f * (sizeBack[0] * sizeBack[2] + sizeBack[1] * sizeBack[0] + sizeBack[2] * sizeBack[1]);
			
			// Calculate total cost.

			const float cost = 0.3f + 1.0f * (costFront * i->nFront + costBack * i->nBack);

			i->cost = cost;
			
		}
		

	}

	bool TreeKd::CalculateSplitPlane(Plane* plane) const
	{

		Assert(plane);
		
		plane->normal[(splitAxis + 0) % 3] = 1.0f;
		plane->normal[(splitAxis + 1) % 3] = 0.0f;
		plane->normal[(splitAxis + 2) % 3] = 0.0f;
		
		std::vector<KdSplitInfo> vSplitInfo;
		
		CalculateSplitVector(vSplitInfo);
		CalculateSplitCosts(vSplitInfo);
		
		int iBest = 0;
		
		for (int i = 1; i < vSplitInfo.size(); ++i)
		{
			if (vSplitInfo[i].d < vSplitInfo[iBest].d)
			{
				iBest = i;
			}
		}
		
		plane->distance = vSplitInfo[iBest].d;
		
		if (vSplitInfo[iBest].nFront == 0 || vSplitInfo[iBest].nBack == 0)
		{
			return true;
		}
		
		return true;

	}
	
}
