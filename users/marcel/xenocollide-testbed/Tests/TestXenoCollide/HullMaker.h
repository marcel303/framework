/*
XenoCollide Collision Detection and Physics Library
Copyright (c) 2007-2014 Gary Snethen http://xenocollide.com

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising
from the use of this software.
Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it freely,
subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must
not claim that you wrote the original software. If you use this
software in a product, an acknowledgment in the product documentation
would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must
not be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
*/

#pragma once

#include <list>
#include <stack>
#include <vector>

#include "Math.h"

#include "ListVector.h"

//////////////////////////////////////////////////////////////////////////////
// HullMaker is a utility class.
//
// It take a set of points and generates a set of faces that form a convex
// hull around the points.  This allows us to render the shape.

class HullMaker
{

public:

	class Edge;

	typedef std::list<Edge*> EdgeList;
	typedef std::list<ListVector> PointList;

	typedef std::vector<Edge*> EdgeVector;

	class Edge
	{
	public:

		Edge(const Vector& _p1, const Vector& _p2, const Vector& _n1, const Vector& _n2, const Vector& _np1, const Vector& _np2, int32 _id1, int32 _id2)
			: p1(_p1), p2(_p2), n1(_n1), n2(_n2), np1(_np1), np2(_np2), id1(_id1), id2(_id2) {}
		Vector p1;
		Vector p2;
		Vector n1;
		Vector n2;
		Vector np1;
		Vector np2;
		int32 id1;
		int32 id2;
	};

	class TreeNode
	{
	public:
		TreeNode();
		void AddPoint( const Vector& p );
		void SplitNode( TreeNode* child1, TreeNode* child2, const Vector& n, const Vector& p );
		Vector minBounds;
		Vector maxBounds;
		std::list<ListVector> pointList;
		EdgeList outsideEdgeList;
	};

	TreeNode* m_points;
	EdgeList m_edgeList;
	EdgeVector m_sortedEdges;
	EdgeList m_faceEdges;
	Vector m_p0;
	Vector m_p1;
	int32 m_vertCount;
	int32 m_faceCount;
	int32 m_faceId;
	std::stack<TreeNode*> m_pointStack;

	HullMaker();
	~HullMaker();
	void Clear();
	void AddPoint(const Vector& p);
	void AddPointToHull(const Vector& p);
	bool AddInitialVert(const Vector& p);
	bool CreateHull();
	void ProcessTreeNode(TreeNode* node);
};
