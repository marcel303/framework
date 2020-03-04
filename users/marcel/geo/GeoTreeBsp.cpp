#include "Debugging.h"
#include "GeoTreeBsp.h"

namespace Geo
{

	TreeBsp::TreeBsp()
		: Tree()
	{

	}

	TreeBsp::~TreeBsp()
	{

	}

	const Plane& TreeBsp::GetSplitPlane() const
	{

		return splitPlane;
		
	}

	void TreeBsp::Generate(int depthLeft, GenerateStatistics* generateStatistics)
	{

		if (depthLeft == 0)
		{
			return;
		}
		
		Plane plane;
		
		if (FindSplitPlane(&plane))
		{
		
			if (generateStatistics)
			{
				generateStatistics->nNodes += 2;
			}
			
			TreeBsp* childFront = new TreeBsp;
			TreeBsp* childBack = new TreeBsp;
			
			Tree::Split(plane, childFront, childBack, generateStatistics);
			
			Assert(childFront->mesh.polys.size() > 0);
			Assert(childBack->mesh.polys.size() > 0);
			
			mesh.Unfinalize();
			mesh.Clear();
			
			splitPlane = plane;
			
			Link(childFront);
			Link(childBack);
			
			childFront->Generate(depthLeft - 1, generateStatistics);
			childBack->Generate(depthLeft - 1, generateStatistics);
			
		}
		else
		{
			
			Assert(mesh.polys.empty() == false);
			
		#if 0
			
			if (mesh->polys.size() > 1)
			{
				printf("---\n");
				for (std::list<Poly*>::iterator i = mesh->polys.begin(); i != mesh->polys.end(); ++i)
				{
					printf("%f %f %f - %f.\n",
						(*i)->plane.normal[0],
						(*i)->plane.normal[1],
						(*i)->plane.normal[2],
						(*i)->plane.distance);
				}
				printf("---\n");
			}
			
		#endif
			
			if (mesh.polys.empty() == false)
			{
				splitPlane = mesh.polys.front()->plane;
			}
		
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

	TreeBsp::SplitInfo::SplitInfo()
	{

		nOn = 0;
		nFront = 0;
		nBack = 0;
		nSpan = 0;

	}

	bool TreeBsp::FindSplitPlane(Plane* plane) const
	{

		Assert(plane);
		
		bool bCandidateFound = false;
		
		SplitInfo splitInfoBest;
		
		for (std::list<Poly*>::const_iterator i = mesh.polys.begin(); i != mesh.polys.end(); ++i)
		{
		
			Plane temp[2];
			
			//const bool bConvex = true;
			const bool bConvex = false;
			
			temp[0] =   (*i)->plane;
			temp[1] = - (*i)->plane;
			
			for (int j = 0; j < (bConvex ? 1 : 2); ++j)
			{
			
				SplitInfo splitInfo = GetSplitInfo(temp[j]);

				if (splitInfo.nFront + splitInfo.nSpan + splitInfo.nOn > 0 && splitInfo.nBack + splitInfo.nSpan > 0)
				{
					
					bool bPromote = false;
					
					if (!bCandidateFound)
					{
					
						bPromote = true;
						
					}
					else
					{
					
						int balance = abs(splitInfo.nBack - splitInfo.nFront) + splitInfo.nSpan;
						int balanceBest = abs(splitInfoBest.nBack - splitInfoBest.nFront) + splitInfo.nSpan;
						
						if (balance < balanceBest)
						{
							bPromote = true;
						}
						else if (balance == balanceBest)
						{
							if (splitInfo.nSpan < splitInfoBest.nSpan)
							{
								bPromote = true;
							}
						}
						
					}
					
					if (bPromote)
					{
					
						*plane = temp[j];
				
						splitInfoBest = splitInfo;
						
						bCandidateFound = true;
						
					}
					
				}
			
			}
		
		}
		
		return bCandidateFound;

	}
	
}
