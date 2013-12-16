#pragma once

#include <algorithm>
#include <math.h>
#include <vector>
#include "Types.h"

#define CLIP_EPS 0.01f
//#define CLIP_EPS 1.0f

class ClipEdge
{
public:
	ClipEdge()
	{
		mIsEmpty = false;
	}

	inline void MarkEmpty()
	{
		mIsEmpty = true;
	}

	void SetupFromPointList(Vec2F point0, Vec2F point1)
	{
		PointList[0] = point0;
		PointList[1] = point1;

		EdgePlane = PlaneF::FromPoints(point0, point1);

		const Vec2F clipPlaneNormal(-EdgePlane.m_Normal[1], +EdgePlane.m_Normal[0]);
		const float clipPlaneDistance = clipPlaneNormal * point0;
		ClipPlane.Set(clipPlaneNormal, clipPlaneDistance);
	}

	void SetupFromPointListEx(Vec2F point0, Vec2F point1, PlaneF edgePlane, PlaneF clipPlane)
	{
		PointList[0] = point0;
		PointList[1] = point1;

		EdgePlane = edgePlane;
		ClipPlane = clipPlane;

		edgePlane.m_Distance = edgePlane.m_Normal.Distance(PointList[0]);
	}

	void ClipByPlane(const PlaneF& plane, ClipEdge& out_front, ClipEdge& out_back) const
	{
		const float d1 = plane.Distance(PointList[0]);
		const float d2 = plane.Distance(PointList[1]);

		const bool isOnFront = d1 >= -CLIP_EPS && d2 >= -CLIP_EPS;
		const bool isOnBack = d1 <= +CLIP_EPS && d2 <= +CLIP_EPS;

		if (isOnBack)
		{
			out_front.MarkEmpty();
			out_back = *this;
		}
		else if (isOnFront)
		{
			out_front = *this;
			out_back.MarkEmpty();
		}
		else if (!isOnFront && !isOnBack)
		{
			// clip

			const float t = - d1 / (d2 - d1);

			const Vec2F midPoint = PointList[0] + (PointList[1] - PointList[0]) * t;

			if (d1 > 0.0f && d2 < 0.0f)
			{
				out_front.SetupFromPointListEx(PointList[0], midPoint, EdgePlane, ClipPlane);
				out_back.SetupFromPointListEx(midPoint, PointList[1], EdgePlane, ClipPlane);
			}
			else if (d1 < 0.0f && d2 > 0.0f)
			{
				out_front.SetupFromPointListEx(midPoint, PointList[1], EdgePlane, ClipPlane);
				out_back.SetupFromPointListEx(PointList[0], midPoint, EdgePlane, ClipPlane);
			}
			else
			{
				assert(false);
			}
		}
		else
		{
			assert(false);
		}
	}

	inline bool IsEmpty_get() const
	{
		return mIsEmpty;
	}

	Vec2F PointList[2];
	PlaneF EdgePlane;
	PlaneF ClipPlane;

private:
	bool mIsEmpty;
};

class ClipPoly
{
public:
	static void PolyToClipEdgeList(std::vector<Vec2F> pointList, std::vector<ClipEdge>& out_edgeList)
	{
		assert(pointList.size() >= 3);
		assert(out_edgeList.size() == 0);

		for (size_t i = 0; i < pointList.size(); ++i)
		{
			Vec2F p0 = pointList[(i + 0) % pointList.size()];
			Vec2F p1 = pointList[(i + 1) % pointList.size()];

			ClipEdge edge;
			edge.SetupFromPointList(p0, p1);

			out_edgeList.push_back(edge);
		}
	}

	void ClipByPlane(const PlaneF& plane, std::vector<ClipEdge>& out_front, std::vector<ClipEdge>& out_back) const
	{
		assert(out_front.size() == 0);
		assert(out_back.size() == 0);

		std::vector<Vec2F> front;
		std::vector<Vec2F> back;

		for (size_t i = 0; i < EdgeList.size(); ++i)
		{
			const ClipEdge& edge = EdgeList[i];

			Vec2F p0 = edge.PointList[0];
			Vec2F p1 = edge.PointList[1];

			const float d0 = plane.Distance(p0);
			const float d1 = plane.Distance(p1);

			const bool isOnFront0 = d0 >= -CLIP_EPS;
			const bool isOnFront1 = d1 >= -CLIP_EPS;
			const bool isOnBack0 = d0 <= +CLIP_EPS;
			const bool isOnBack1 = d1 <= +CLIP_EPS;
			const bool isInMid = d0 >= -CLIP_EPS && d0 <= +CLIP_EPS;

			if (isInMid)
			{
				front.push_back(p0);
				back.push_back(p1);
			}
			else if (isOnFront0 && isOnFront1)
			{
				front.push_back(p0);
			}
			else if (isOnBack0 && isOnBack1)
			{
				back.push_back(p0);
			}
			else if (true)// if (!isOnFront && !isOnBack)
			{
				// clip

				const float t = - d0 / (d1 - d0);

				const Vec2F midPoint = p0 + (p1 - p0) * t;

				if (d0 > 0.0f && d1 < 0.0f)
				{
					front.push_back(edge.PointList[0]);
					front.push_back(midPoint);
					back.push_back(midPoint);
				}
				else if (d0 < 0.0f && d1 > 0.0f)
				{
					back.push_back(edge.PointList[0]);
					back.push_back(midPoint);
					front.push_back(midPoint);
				}
				else
				{
					assert(false);
				}
			}
			else
			{
				assert(false);
			}
		}

		PolyToClipEdgeList(front, out_front);
		PolyToClipEdgeList(back, out_back);
	}

	std::vector<ClipEdge> EdgeList;
};

class ClipShape
{
public:
	void ClipByPlane(const PlaneF& plane, ClipShape& out_front, ClipShape& out_back) const
	{
		assert(EdgeList.size() > 0);
		assert(out_front.EdgeList.size() == 0);
		assert(out_back.EdgeList.size() == 0);

		bool isOnFront = true;
		bool isOnBack = true;

		for (size_t i = 0; i < EdgeList.size(); ++i)
		{
			const ClipEdge& edge = EdgeList[i];

			for (int j = 0; j < 2; ++j)
			{
				const float d = plane.Distance(edge.PointList[j]);

				isOnFront &= d >= -CLIP_EPS;
				isOnBack &= d <= +CLIP_EPS;
			}
		}

		if (isOnBack)
		{
			out_back = *this;
			return;
		}
		else if (isOnFront)
		{
			out_front = *this;
			return;
		}
		else if (!isOnFront && !isOnBack)
		{
#if 0
			for (size_t i = 0; i < EdgeList.size(); ++i)
			{
				const ClipEdge& edge = EdgeList[i];

				ClipEdge edgeFront;
				ClipEdge edgeBack;

				edge.ClipByPlane(plane, edgeFront, edgeBack);

				if (!edgeFront.IsEmpty_get())
				{
					out_front.EdgeList.push_back(edgeFront);
				}

				if (!edgeBack.IsEmpty_get())
				{
					out_back.EdgeList.push_back(edgeBack);
				}
			}
#else
			ClipPoly poly;
			poly.EdgeList = EdgeList;
			poly.ClipByPlane(plane, out_front.EdgeList, out_back.EdgeList);
#endif
		}
		else
		{
			assert(false);
		}
	}

	inline bool IsEmpty_get() const
	{
		return EdgeList.empty();
	}

	std::vector<ClipEdge> EdgeList;
};

class BeamNode
{
public:
	BeamNode()
	{
		Parent = 0;
		ChildFront = 0;
		ChildBack = 0;
		IsFilled = false;
	}

	BeamNode(BeamNode* parent, ClipEdge edge)
	{
		Parent = parent;
		ChildFront = 0;
		ChildBack = 0;
		ClipEdge = edge;
		IsFilled = false;
	}

	~BeamNode()
	{
		delete ChildFront;
		ChildFront = 0;
		delete ChildBack;
		ChildBack = 0;
	}

	void AddEdgeListFilled(const ClipShape& shape, int offset, int side)
	{
		bool isLast = offset == shape.EdgeList.size() - 1;

		BeamNode* node = new BeamNode(this, shape.EdgeList[offset]);
		node->IsFilled = isLast;

		if (offset == 0 && side == -1)
			ChildBack = node;
		else
			ChildFront = node;

		if (!isLast)
		{
			node->AddEdgeListFilled(shape, offset + 1, side);
		}
	}

	void AddShapeClipped(const ClipShape& shape)
	{
		ClipShape front;
		ClipShape back;

		shape.ClipByPlane(ClipEdge.ClipPlane, front, back);

		if (!front.IsEmpty_get())
		{
			if (!IsFilled)
			{
				if (!ChildFront)
				{
					AddEdgeListFilled(front, 0, +1);
				}
				else
				{
					ChildFront->AddShapeClipped(front);
				}
			}
		}
		if (!back.IsEmpty_get())
		{
			//printf("back!!\n");

			if (!ChildBack)
			{
				AddEdgeListFilled(back, 0, -1);
			}
			else
			{
				ChildBack->AddShapeClipped(back);
			}
		}
	}

	ClipEdge ClipEdge;
	BeamNode* Parent;
	BeamNode* ChildFront;
	BeamNode* ChildBack;
	bool IsFilled;
};

static BeamNode* CreateBeamNode(BeamNode* parent, Vec2F p0, Vec2F p1, bool isFilled)
{
	ClipEdge edge;
	edge.SetupFromPointList(p0, p1);

	BeamNode* node = new BeamNode(parent, edge);
	node->IsFilled = isFilled;

	return node;
}

class BeamTree
{
public:
	BeamTree()
	{
		Root = 0;
	}

	~BeamTree()
	{
		delete Root;
		Root = 0;
	}

	void Init(float x, float y, float sx, float sy)
	{
		Root = CreateBeamNode(0, Vec2F(x + sx, y), Vec2F(x, y), true);
#if 1
		Root->ChildBack = CreateBeamNode(Root, Vec2F(x + sx, y + sy), Vec2F(x + sx, y), true);
		Root->ChildBack->ChildBack = CreateBeamNode(Root->ChildBack, Vec2F(x, y + sy), Vec2F(x + sx, y + sy), true);
		Root->ChildBack->ChildBack->ChildBack = CreateBeamNode(Root->ChildBack->ChildBack, Vec2F(x, y), Vec2F(x, y + sy), true);
#endif
	}

	void AddClipped(const ClipShape& shape)
	{
		Root->AddShapeClipped(shape);
	}

	bool IsVisible_Point(const Vec2F& p) const
	{
		const BeamNode* node = Root;

		while (true)
		{
			const float d = node->ClipEdge.ClipPlane.Distance(p);
			
			if (d >= 0.0f)
			{
				node = node->ChildFront;

				if (!node)
					return false;
			}
			else
			{
				node = node->ChildBack;

				if (!node)
					return true;
			}
		}
	}

	bool IsVisible_Circle(Vec2F p, float r) const
	{
		std::stack<const BeamNode*> stack;
		stack.push(Root);

		while (stack.size() > 0)
		{
			const BeamNode* node = stack.top();
			stack.pop();

			const float d = node->ClipEdge.ClipPlane.Distance(p);
			
			if (d <= +r && !node->ChildBack)
			{
#if DEBUG
				printf("p: (%03.2f, %03.2f) x %03.2f\n",
					p[0],
					p[1],
					r);
				printf("d: %03.2f\n", d);
				printf("clip plane: (%03.2f, %03.2f) @ %03.2f\n",
					node->ClipEdge.ClipPlane.m_Normal[0],
					node->ClipEdge.ClipPlane.m_Normal[1],
					node->ClipEdge.ClipPlane.m_Distance);
#endif
				return true;
			}

			if (d >= -r && node->ChildFront)
				stack.push(node->ChildFront);
			if (d <= +r && node->ChildBack)
				stack.push(node->ChildBack);
		}

		return false;
	}

#if 0
	class PPHelper
	{
	public:
		void Add(BeamNode* node)
		{
			NodeList.push_back(node);
		}

		void Remove(BeamNode* node)
		{
			NodeList.erase(std::find(NodeList.begin(), NodeList.end(), node));
		}

		bool IsRedundant(BeamNode* node)
		{
			for (size_t i = 0; i < NodeList.size(); ++i)
			{
				BeamNode* node2 = NodeList[i];

				if (node2 == node)
					continue;

				if (IsRedundant(node->ClipEdge, node2->ClipEdge))
					return true;
			}

			return false;
		}

		bool IsRedundant(const ClipEdge& child, const ClipEdge& parent)
		{
			const float cd0 = parent.ClipPlane.Distance(child.PointList[0]);
			const float cd1 = parent.ClipPlane.Distance(child.PointList[1]);

			if (cd0 < -CLIP_EPS || 
				cd0 > +CLIP_EPS ||
				cd1 < -CLIP_EPS ||
				cd1 > +CLIP_EPS)
				return false;

			const float edMin = 0.0f;
			const float edMax = parent.PointList[0].DistanceTo(parent.PointList[1]);

			float ed0 = parent.EdgePlane.Distance(child.PointList[0]);
			float ed1 = parent.EdgePlane.Distance(child.PointList[1]);

			if (ed0 > ed1)
				std::swap(ed0, ed1);

			if (ed0 < edMin -CLIP_EPS || 
				ed1 > edMax + CLIP_EPS)
				return false;

			return true;
		}

		std::vector<BeamNode*> NodeList;
	};

	void Postprocess()
	{
		PPHelper helper;

		std::stack<BeamNode*> stack;
		stack.push(Root);

		while (stack.size() > 0)
		{
			BeamNode* node = stack.top();
			stack.pop();

			//if (!node->ChildFront && !node->ChildBack)// && node->IsFilled)
			//if (node->IsFilled)
			if (true)
			{
				printf("pp\n");

				helper.Add(node);
			}

			if (node)
			{
				if (node->ChildFront)
					stack.push(node->ChildFront);
				if (node->ChildBack)
					stack.push(node->ChildBack);
			}
		}

		std::vector<BeamNode*> nodeList = helper.NodeList;

		for (size_t i = 0; i < nodeList.size(); ++i)
		{
			BeamNode* node = nodeList[i];

			if (helper.IsRedundant(node) || true)
			{
				helper.Remove(node);

				node->IsFilled = false;

				/*if (node->Parent->ChildBack == node)
					node->Parent->ChildBack = 0;
				if (node->Parent->ChildFront == node)
					node->Parent->ChildFront = 0;

				delete node;
				node = 0;*/
			}
		}
	}
#endif

	BeamNode* Root;
};
