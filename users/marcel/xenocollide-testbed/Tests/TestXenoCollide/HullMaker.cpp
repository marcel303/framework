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

#include "HullMaker.h"

#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <stack>

using namespace std;

namespace XenoCollide
{
	static const float32 kCoplanarEpsilon = 0;//0.0001f;
	static const float32 kCoplanarEpsilon2 = 0;//.0001f;

#ifdef ASSERT
#undef ASSERT
#endif
#define ASSERT(a) if ((a) == false) { *(int32*)NULL = 0; }

	HullMaker::HullMaker()
	{
		m_points = new TreeNode();
	}

	HullMaker::~HullMaker()
	{
		Clear();

		delete m_points;
		m_points = NULL;
	}

	void HullMaker::Clear()
	{
		while (!m_edgeList.empty())
		{
			delete m_edgeList.back();
			m_edgeList.pop_back();
		}

		while (!m_sortedEdges.empty())
		{
			delete m_sortedEdges.back();
			m_sortedEdges.pop_back();
		}

		while (!m_faceEdges.empty())
		{
			delete m_faceEdges.back();
			m_faceEdges.pop_back();
		}

		while (!m_pointStack.empty())
		{
			delete m_pointStack.top();
			m_pointStack.pop();
		}

		delete m_points;
		m_points = new TreeNode();
	}

	void HullMaker::AddPoint(const Vector& p)
	{
		// XENO TEST: Reject any points that are extremely close to existing points
		for (list<ListVector>::iterator pit = m_points->pointList.begin(); pit != m_points->pointList.end(); pit++)
		{
			if ((p - (Vector)(*pit)).Len3() < 0.01f)
			{
				return;
			}
		}

		m_points->AddPoint(p);
	}

	static bool EdgeGreaterThan(HullMaker::Edge* a, HullMaker::Edge* b)
	{
		if (a->id1 > b->id1) return true;

#if 0
		if (a->n1.X() > b->n1.X()) return true;
		if (a->n1.X() < b->n1.X()) return false;

		if (a->n1.Y() > b->n1.Y()) return true;
		if (a->n1.Y() < b->n1.Y()) return false;

		if (a->n1.Z() > b->n1.Z()) return true;
		if (a->n1.Z() < b->n1.Z()) return false;
#endif

		return false;
	}

	bool HullMaker::CreateHull()
	{
		m_faceId = 0;
		m_vertCount = 0;

		if (m_points->pointList.size() < 4) return false;

		// Build the initial tetrahedron from four points
		int32 pointsRemaining = (int32)m_points->pointList.size();
		Vector p;
		do
		{
			p = m_points->pointList.front();
			m_points->pointList.pop_front();
		} while (AddInitialVert(p) && --pointsRemaining > 0);

		if (pointsRemaining <= 0) return false;

		AddPointToHull(p);

		// Process the node tree depth-first using a stack
		m_pointStack.push(m_points);
		m_points = NULL;
		while (!m_pointStack.empty())
		{
			TreeNode* node = m_pointStack.top();
			m_pointStack.pop();
			ProcessTreeNode(node);
		}

		// Move all edges to an array
		while (!m_edgeList.empty())
		{
			Edge* e = m_edgeList.front();
			m_edgeList.pop_front();

			m_sortedEdges.push_back(e);

			// Create a complement for each edge
			Edge* newEdge = new Edge(e->p2, e->p1, e->n2, e->n1, e->np2, e->np1, e->id2, e->id1);
			m_sortedEdges.push_back(newEdge);
		}

		// Sort the edges by normal
		sort(m_sortedEdges.begin(), m_sortedEdges.end(), EdgeGreaterThan);

		// Create faces from the edges
		set<ListVector> vertSet;
		m_faceCount = 0;

		while (!m_sortedEdges.empty())
		{
			// Extract the edges for one face
			EdgeList faceEdgesRaw;
			Edge* e = m_sortedEdges.back();
			Vector n = e->n1;
			int32 id = e->id1;
			do
			{
				m_sortedEdges.pop_back();
				faceEdgesRaw.push_back(e);
				if (m_sortedEdges.empty())
				{
					e = NULL;
				}
				else
				{
					e = m_sortedEdges.back();
				}
			} while (e != NULL && e->id1 == id);

			// Remove degenerate edges
			EdgeList faceEdges;
			while (!faceEdgesRaw.empty())
			{
				e = faceEdgesRaw.front();
				faceEdgesRaw.pop_front();

				for (EdgeList::iterator edgeIt = faceEdgesRaw.begin(); edgeIt != faceEdgesRaw.end(); edgeIt++)
				{
					// If the edge is degenerate, delete it and its complement
					if ((*edgeIt)->p1 == e->p2 && (*edgeIt)->p2 == e->p1)
					{
						delete e;
						e = *edgeIt;
						faceEdgesRaw.erase(edgeIt);
						delete e;
						e = NULL;
						break;
					}
				}

				// If the edge was not degenerate, place it into the new list
				if (e != NULL)
				{
					faceEdges.push_back(e);
				}
			}

			// Sort the face edges into CCW order and place them into m_faceEdges
			e = faceEdges.front();
			faceEdges.pop_front();
			m_faceEdges.push_back(e);
			vertSet.insert(e->p1);

			while (!faceEdges.empty())
			{
				for (EdgeList::iterator edgeIt = faceEdges.begin(); edgeIt != faceEdges.end(); edgeIt++)
				{
					if ((*edgeIt)->p1 == e->p2)
					{
						e = *edgeIt;
						faceEdges.erase(edgeIt);

						m_faceEdges.push_back(e);
						vertSet.insert(e->p1);
						break;
					}
				}
			}

			m_faceCount++;
		}

		m_vertCount = (int32)vertSet.size();

		return true;
	}

	class ListVectorPair
	{
	public:
		ListVectorPair(const ListVector& a, const ListVector& b) : p1(a), p2(b) { }
		ListVector p1;
		ListVector p2;
	};

	bool operator < (const ListVectorPair& a, const ListVectorPair& b)
	{
		if (a.p1 < b.p1) return true;
		if (b.p1 < a.p1) return false;
		if (a.p2 < b.p2) return true;
		return false;
	}

	// Add a new point to the hull, removing any verts and faces that wind up inside the hull
	void HullMaker::AddPointToHull(const Vector& p)
	{
		if (m_vertCount < 4)
		{
			if (AddInitialVert(p)) return;
		}

		list<EdgeList::iterator> removedEdges;
		list<Edge*> newHalfEdges;

		for (EdgeList::iterator edgeIt = m_edgeList.begin(); edgeIt != m_edgeList.end(); edgeIt++)
		{
			Edge* edge = *edgeIt;

			ASSERT((p - edge->p1).Len3() > 0.00001f);
			ASSERT((p - edge->p2).Len3() > 0.00001f);

			bool side1Inside = (edge->n1 * (p - edge->np1) > -kCoplanarEpsilon);
			bool side2Inside = (edge->n2 * (p - edge->np2) > -kCoplanarEpsilon);

			if (side1Inside)
			{
				if (side2Inside)
				{
					// Both faces are inside the hull, so remove the edge
					removedEdges.push_back(edgeIt);
				}
				else
				{
					// face 1 is inside, but 2 is outside -- insert two new edges
					Vector n = (edge->p2 - edge->p1) % (p - edge->p1);

					n.Normalize3();

					int32 id = m_faceId++;

					Vector np = p;

					if (Abs(1 - (edge->n2 * n)) < kCoplanarEpsilon2)
					{
						n = edge->n2;
						id = edge->id2;
						np = edge->np2;
						removedEdges.push_back(edgeIt);
					}
					else
					{
						edge->n1 = n;
						edge->np1 = np;
						edge->id1 = id;
					}

					Edge* newEdge = new Edge(edge->p2, p, n, Vector(0, 0, 0), np, Vector(0, 0, 0), id, -1);
					newHalfEdges.push_back(newEdge);

					newEdge = new Edge(p, edge->p1, n, Vector(0, 0, 0), np, Vector(0, 0, 0), id, -1);
					newHalfEdges.push_back(newEdge);
				}
			}
			else
			{
				if (side2Inside)
				{
					// face 2 is inside, but 1 is outside -- insert two new edges
					Vector n = (edge->p1 - edge->p2) % (p - edge->p2);

					n.Normalize3();

					int32 id = m_faceId++;

					Vector np = p;

					if (Abs(1 - (edge->n1 * n)) < kCoplanarEpsilon2)
					{
						n = edge->n1;
						id = edge->id1;
						np = edge->np1;
						removedEdges.push_back(edgeIt);
					}
					else
					{
						edge->n2 = n;
						edge->np2 = np;
						edge->id2 = id;
					}

					Edge* newEdge = new Edge(edge->p1, p, n, Vector(0, 0, 0), np, Vector(0, 0, 0), id, -1);
					newHalfEdges.push_back(newEdge);

					newEdge = new Edge(p, edge->p2, n, Vector(0, 0, 0), np, Vector(0, 0, 0), id, -1);
					newHalfEdges.push_back(newEdge);

				}
			}
		}

		while (!removedEdges.empty())
		{
			Edge* e = *(removedEdges.front());
			m_edgeList.erase(removedEdges.front());
			removedEdges.pop_front();
			delete e;
		}

		// XENO: Disabled due to assertion being hit frequently
		if (0 && newHalfEdges.size() > 15)
		{
			// Put together the half edges
			map<ListVectorPair, Edge*> halfEdgeMap;

			for (EdgeList::iterator it = newHalfEdges.begin(); it != newHalfEdges.end(); it++)
			{
				map<ListVectorPair, Edge*>::iterator lookFor = halfEdgeMap.find(ListVectorPair((*it)->p1, (*it)->p2));
				ASSERT(halfEdgeMap.find(ListVectorPair((*it)->p1, (*it)->p2)) == halfEdgeMap.end());
				halfEdgeMap[ListVectorPair((*it)->p1, (*it)->p2)] = (*it);
			}
			newHalfEdges.clear();

			while (!halfEdgeMap.empty())
			{
				Edge* halfEdge = (*halfEdgeMap.begin()).second;
				halfEdgeMap.erase(halfEdgeMap.begin());

				map<ListVectorPair, Edge*>::iterator otherHalfIt = halfEdgeMap.find(ListVectorPair(halfEdge->p2, halfEdge->p1));
				if (otherHalfIt == halfEdgeMap.end())
				{
					*(int32*)NULL = 0;
				}
				Edge* otherHalfEdge = (*otherHalfIt).second;
				halfEdgeMap.erase(otherHalfIt);
				halfEdge->n2 = otherHalfEdge->n1;
				halfEdge->np2 = otherHalfEdge->np1;
				halfEdge->id2 = otherHalfEdge->id1;
				delete otherHalfEdge;

				if (halfEdge->id1 == halfEdge->id2)
				{
					delete halfEdge;
				}
				else
				{
					m_edgeList.push_back(halfEdge);
				}
			}
		}
		else
		{
			while (!newHalfEdges.empty())
			{
				Edge* halfEdge = newHalfEdges.front();
				newHalfEdges.pop_front();

				EdgeList::iterator edgeIt = newHalfEdges.begin();
				while (edgeIt != newHalfEdges.end())
				{
					if ((*edgeIt)->p1 == halfEdge->p2 && (*edgeIt)->p2 == halfEdge->p1)
					{
						halfEdge->n2 = (*edgeIt)->n1;
						halfEdge->np2 = (*edgeIt)->np1;
						halfEdge->id2 = (*edgeIt)->id1;
						newHalfEdges.erase(edgeIt);
						break;
					}
					edgeIt++;
				}

				//			if (edgeIt == newHalfEdges.end()) { *(int32*)NULL = 0; }
				//			ASSERT( edgeIt != newHalfEdges.end() );

				if (halfEdge->id1 == halfEdge->id2)
				{
					delete halfEdge;
				}
				else
				{
					m_edgeList.push_back(halfEdge);
				}
			}
		}
	}

	bool HullMaker::AddInitialVert(const Vector& p)
	{
		switch (m_vertCount)
		{
		case 0:
			m_p0 = p;
			m_vertCount++;
			break;

		case 1:
			if (p != m_p0)
			{
				m_p1 = p;
				m_vertCount++;
			}
			else
			{
				m_points->pointList.push_back(p);
			}

			break;

		case 2:
		{
			Vector n = (m_p1 - m_p0) % (p - m_p0);
			if (n.CanNormalize3())
			{
				n.Normalize3();

				int32 id1 = m_faceId++;
				int32 id2 = m_faceId++;

				Edge* e = new Edge(m_p0, m_p1, n, -n, p, p, id1, id2);
				m_edgeList.push_back(e);

				e = new Edge(m_p1, p, n, -n, p, p, id1, id2);
				m_edgeList.push_back(e);

				e = new Edge(p, m_p0, n, -n, p, p, id1, id2);
				m_edgeList.push_back(e);

				m_vertCount++;
			}
			else
			{
				m_points->pointList.push_back(p);
			}
		}
		break;

		case 3:
		{
			Edge* e = m_edgeList.front();
			if (Abs(e->n1 * (p - e->np1)) > kCoplanarEpsilon2)
			{
				m_vertCount++;
				return false;
			}
			else
			{
				m_points->pointList.push_back(p);
			}
		}
		break;
		}

		return true;
	}

	void HullMaker::ProcessTreeNode(TreeNode* node)
	{
		// If the point list is empty, we just need to add the outside edges back to the pool
		if (node->pointList.empty())
		{
			m_edgeList.insert(m_edgeList.end(), node->outsideEdgeList.begin(), node->outsideEdgeList.end());
			node->outsideEdgeList.clear();
			delete node;
			return;
		}

		Vector midPoint = (node->minBounds + node->maxBounds) * 0.5f;
		Vector radius = (node->maxBounds - node->minBounds) * 0.5f;

		EdgeList::iterator edgeIt = m_edgeList.begin();
		EdgeList::iterator oldEdgeIt;
		while (edgeIt != m_edgeList.end())
		{
			Edge* e = *edgeIt;
			Vector absNormal1 = CompAbs(e->n1);
			Vector absNormal2 = CompAbs(e->n2);
			bool inside1 = ((midPoint - e->np1) * e->n1) > (absNormal1 * radius) + 0.01f;
			bool inside2 = ((midPoint - e->np2) * e->n2) > (absNormal2 * radius) + 0.01f;
			bool outside1 = ((midPoint - e->np1) * e->n1) < -(absNormal1 * radius) - 0.01f;
			bool outside2 = ((midPoint - e->np2) * e->n2) < -(absNormal2 * radius) - 0.01f;

			oldEdgeIt = edgeIt++;

			if (inside1 && inside2)
			{
				m_edgeList.erase(oldEdgeIt);
				delete e;
			}
			else if (outside1 && outside2)
			{
				m_edgeList.erase(oldEdgeIt);
				node->outsideEdgeList.push_back(e);
			}
		}

		if (node->pointList.size() > 150)
		{
			// Calculate a splitting plane that divides this node in half along the longest dimension
			Vector n(1, 0, 0);
			if (radius.Y() > radius.Z())
			{
				if (radius.Y() > radius.X())
				{
					n = Vector(0, 1, 0);
				}
			}
			else if (radius.Z() > radius.X())
			{
				n = Vector(0, 0, 1);
			}

			// Split the node along the plane
			TreeNode* newNode1 = new TreeNode();
			TreeNode* newNode2 = new TreeNode();
			node->SplitNode(newNode1, newNode2, n, midPoint);

			if (node->outsideEdgeList.empty())
			{
				delete node;
			}
			else
			{
				m_pointStack.push(node);
			}
			m_pointStack.push(newNode1);
			m_pointStack.push(newNode2);
		}
		else
		{
			while (!node->pointList.empty())
			{
				Vector p = node->pointList.front();
				node->pointList.pop_front();
				AddPointToHull(p);
			}

			m_edgeList.insert(m_edgeList.end(), node->outsideEdgeList.begin(), node->outsideEdgeList.end());
			node->outsideEdgeList.clear();

			delete node;
		}
	}

	HullMaker::TreeNode::TreeNode()
	{
		minBounds = Vector(MAX_FLOAT, MAX_FLOAT, MAX_FLOAT);
		maxBounds = Vector(MIN_FLOAT, MIN_FLOAT, MIN_FLOAT);
	}

	void HullMaker::TreeNode::AddPoint(const Vector& p)
	{
		minBounds = CompMin(minBounds, p);
		maxBounds = CompMax(maxBounds, p);
		pointList.push_back(p);
	}

	void HullMaker::TreeNode::SplitNode(TreeNode* child1, TreeNode* child2, const Vector& n, const Vector& p)
	{
		PointList::iterator point = pointList.begin();
		while (point != pointList.end())
		{
			if (((*point) - p) * n > 0)
			{
				child1->AddPoint((*point));
			}
			else
			{
				child2->AddPoint((*point));
			}

			PointList::iterator old = point;
			point++;
			pointList.erase(old);
		}
	}
}
