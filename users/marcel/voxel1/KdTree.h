#pragma once

#include "SIMD.h"

template <typename T>
class Extents
{
public:
	Extents()
	{
		SetMinSize(0, 0, 0, 0, 0, 0);
	}

	Extents(T minX, T minY, T minZ, T sizeX, T sizeY, T sizeZ)
	{
		SetMinSize(minX, minY, minZ, sizeX, sizeY, sizeZ);
	}

	void SetMinSize(T minX, T minY, T minZ, T sizeX, T sizeY, T sizeZ)
	{
		m_Min[0] = minX;
		m_Min[1] = minY;
		m_Min[2] = minZ;
		m_Max[0] = minX + sizeX - 1;
		m_Max[1] = minY + sizeY - 1;
		m_Max[2] = minZ + sizeZ - 1;
	}

	inline int MajorAxis_get() const
	{
		int result = 0;

		BaseVec<T> size = Size_get();

		if (size[1] > size[result])
			result = 1;
		if (size[2] > size[result])
			result = 2;

		return result;
	}

	inline BaseVec<T> Mid_get() const
	{
		return (m_Min + m_Max) / 2;
	}

	inline BaseVec<T> Size_get() const
	{
		return m_Max - m_Min + BaseVec<T>(1, 1, 1);
	}

	void Split(int axis, T pos, Extents<T>* extents1, Extents<T>* extents2) const
	{
		*extents1 = *this;
		*extents2 = *this;

		extents1->m_Max[axis] = pos - 1;
		extents2->m_Min[axis] = pos;
	}

	T Area_get() const
	{
		T result = (T)0;

		BaseVec<T> size = Size_get();

		for (int i = 0; i < 3; ++i)
		{
			int axis1 = (i + 0) % 3;
			int axis2 = (i + 1) % 3;

			T area = size[axis1] * size[axis2] * 2;

			result += area;
		}

		return result;
	}

	inline BOOL Inside(const BaseVec<T>& pos) const
	{
		for (int i = 0; i < 3; ++i)
		{
			if (pos[i] < m_Min[i])
				return FALSE;
			if (pos[i] > m_Max[i])
				return FALSE;
		}

		return TRUE;
	}

	BaseVec<T> m_Min;
	BaseVec<T> m_Max;
};

typedef Extents<int> ExtentsI;

class ExtentsF
{
public:
	ExtentsF()
	{
		m_Min = m_Max = SimdVec(VZERO);
	}

	inline BOOL Inside(SimdVecArg pos) const
	{
		return pos.ALL_GE3(m_Min) && pos.ALL_LE3(m_Max);
	}

	inline int MajorAxis_get() const
	{
		int result = 0;

		SimdVec size = Size_get();

		if (size(1) > size(result))
			result = 1;
		if (size(2) > size(result))
			result = 2;

		return result;
	}

	inline SimdVec Mid_get() const
	{
		return m_Min.Add(m_Max).Mul(SimdVec(0.5f));
	}

	inline SimdVec Size_get() const
	{
		return m_Max.Sub(m_Min).Add(SimdVec(VONE));
	}

	SimdVec m_Min;
	SimdVec m_Max;
};

inline SimdVec ToSimdVec(const VecI & v)
{
	return SimdVec(v[0], v[1], v[2]);
}

inline SimdVec ToSimdVec(const VecF & v)
{
	return SimdVec(&v[0]);
}

inline VecF ToVecF(SimdVecArg v)
{
	return VecF(v(0), v(1), v(2));
}

class KdNode
{
public:
	KdNode()
	{
		Initialize(0);
	}

	KdNode(const ExtentsI& extents)
	{
		Initialize(&extents);
	}

	void Initialize(const ExtentsI* extents)
	{
		m_Child[0] = 0;
		m_Child[1] = 0;

		m_IsEmpty = FALSE;
		m_IsSolid = TRUE;

		if (extents)
		{
			m_Extents = *extents;
		}
	}

	int CountVoxi_Solid(Map* map, ExtentsI extents) const
	{
		int result = 0;

		for (int x = extents.m_Min[0]; x <= extents.m_Max[0]; ++x)
		{
			for (int y = extents.m_Min[1]; y <= extents.m_Max[1]; ++y)
			{
				for (int z = extents.m_Min[2]; z <= extents.m_Max[2]; ++z)
				{
					if (map->m_Voxels[x][y][z].IsSolid)
						result++;
				}
			}
		}

		return result;
	}

	class SplitInfo
	{
	public:
		void Initialize(int axis, int pos)
		{
			m_Axis = axis;
			m_Pos = pos;
		}

		int m_Axis;
		int m_Pos;
	};

	bool ShrinkExtents(Map* map, const ExtentsI& extents, ExtentsI* o_Result) const
	{
		VecI min(-1, -1, -1);
		VecI max(-1, -1, -1);

		for (int x = extents.m_Min[0]; x <= extents.m_Max[0]; ++x)
		{
			for (int y = extents.m_Min[1]; y <= extents.m_Max[1]; ++y)
			{
				for (int z = extents.m_Min[2]; z <= extents.m_Max[2]; ++z)
				{
					int isSolid = map->m_Voxels[x][y][z].IsSolid;

					if (isSolid)
					{
						VecI pos(x, y, z);

						for (int i = 0; i < 3; ++i)
						{
							if (pos[i] < min[i] || min[i] == -1)
								min[i] = pos[i];
							if (pos[i] > max[i] || max[i] == -1)
								max[i] = pos[i];
						}
					}
				}
			}
		}

		if (min[0] == -1)
			return false;

		o_Result->m_Min = min;
		o_Result->m_Max = max;

		return true;
	}

	BOOL GetSplitPos(Map* map, int axis, SplitInfo* info) const
	{
		return GetSplitPos2(map, axis, info);
	}

	BOOL GetSplitPos2(Map* map, int _axis, SplitInfo* info) const
	{
		class BestInfo
		{
		public:
			inline BestInfo()
			{
				hueristic = -1.0f;
			}

			inline BOOL IsSet() const
			{
				return hueristic >= 0.0f;
			}

			float hueristic;
			int axis;
			int pos;
			int count1;
			int count2;
		};

		BestInfo best;

//		for (int axis = 0; axis < 3; ++axis)
		int axis = _axis;
		{
			for (int i = m_Extents.m_Min[axis]; i <= m_Extents.m_Max[axis]; ++i)
			{
				ExtentsI extents1;
				ExtentsI extents2;

				m_Extents.Split(axis, i, &extents1, &extents2);

				bool solid1 = ShrinkExtents(map, extents1, &extents1);
				bool solid2 = ShrinkExtents(map, extents2, &extents2);

				if (!solid1 || !solid2)
					continue;

				int area1 = extents1.Area_get();
				int area2 = extents2.Area_get();

				if (area1 == 0 || area2 == 0)
					continue;

				int count1 = CountVoxi_Solid(map, extents1);
				int count2 = CountVoxi_Solid(map, extents2);

				if (count1 == 0 || count2 == 0)
					continue;

#if 0
				count1 = count1 * count1;
				count2 = count2 * count2;
#endif

				float hueristic = area1 * count1 + area2 * count2;
				//float hueristic = area1 / (float)count1 + area2 / (float)count2;

				if (hueristic < best.hueristic || !best.IsSet())
				{
					best.hueristic = hueristic;
					best.axis = axis;
					best.pos = i;
					best.count1 = count1;
					best.count2 = count2;
				}
			}
		}

		if (!best.IsSet())
			return FALSE;

		Assert(best.pos != -1);
		Assert(best.count1 > 0);
		Assert(best.count2 > 0);

		info->Initialize(best.axis, best.pos);

		return TRUE;
	}

	BOOL GetSplitPos1(Map* map, int axis, SplitInfo* info) const
	{
		int fc = 0;
		int bc = CountVoxi_Solid(map, m_Extents);

		// Node is empty?

		if (bc == 0)
			return FALSE;

		// Split is trivial?

		if (m_Extents.Size_get()[axis] == 2)
		{
			info->Initialize(axis, m_Extents.m_Min[axis] + 1);

			return TRUE;
		}

		int axisA = (axis + 1) % 3;
		int axisB = (axis + 2) % 3;

		int bestHueristic = -1;
		int bestPos = -1;
		
		for (int i = m_Extents.m_Min[axis] + 1; i <= m_Extents.m_Max[axis] - 1; ++i)
		{
			int sliceCount = 0;

			for (int a = m_Extents.m_Min[axisA]; a <= m_Extents.m_Max[axisA]; ++a)
			{
				for (int b = m_Extents.m_Min[axisB]; b <= m_Extents.m_Max[axisB]; ++b)
				{
					VecI pos;

					pos[axis] = i;
					pos[axisA] = a;
					pos[axisB] = b;

					if (map->m_Voxels[pos[0]][pos[1]][pos[2]].IsSolid)
						sliceCount++;
				}
			}

			if (sliceCount > 0)
			{
				fc += sliceCount;
				bc -= sliceCount;
			}

			int hueristic = abs(bc - fc);

			if (hueristic < bestHueristic || bestHueristic == -1)
			{
				bestHueristic = hueristic;
				bestPos = i;
			}
		}

		Assert(bestPos != -1);

		info->Initialize(axis, bestPos);

		return TRUE;
	}

	void SplitRecur(Map* map)
	{
		ShrinkExtents(map, m_Extents, &m_Extents);
		m_BB.m_Min = ToSimdVec(m_Extents.m_Min);
		m_BB.m_Max = ToSimdVec(m_Extents.m_Min + m_Extents.Size_get());

		{
			SimdVec mid = m_BB.Mid_get();
			SimdVec size = m_BB.Size_get();

			m_bbSphere.Initialize(
				ToVecF(mid),
				size.Len3().X() / 2.0f);
		}

#if 1
		int axis = m_BB.MajorAxis_get();
#endif

		SplitInfo info;

		if (!GetSplitPos(map, axis, &info))
			return;

		//info.m_Pos -= m_Extents.m_Min[axis];

		Assert(info.m_Pos > m_Extents.m_Min[info.m_Axis]);
		Assert(info.m_Pos <= m_Extents.m_Max[info.m_Axis]);

		ExtentsI extents1;
		ExtentsI extents2;

		m_Extents.Split(info.m_Axis, info.m_Pos, &extents1, &extents2);

		//

		m_Plane.m_Normal.SetZero();
		m_Plane.m_Normal(info.m_Axis) = 1.0f;
		m_Plane.m_Distance.SetAll(info.m_Pos);

		Assert(
			m_Plane.m_Distance > m_Extents.m_Min[info.m_Axis] &&
			m_Plane.m_Distance <= m_Extents.m_Max[info.m_Axis]);

#if 0
		LOG("Split: Plane: %d %d %d, %d",
			m_Plane.m_Normal[0],
			m_Plane.m_Normal[1],
			m_Plane.m_Normal[2],
			m_Plane.m_Distance);
		LOG("Split: Parent: Min: %d %d %d",
			m_Extents.m_Min[0],
			m_Extents.m_Min[1],
			m_Extents.m_Min[2]);
		LOG("Split: Parent: Size: %d %d %d",
			m_Extents.m_Size[0],
			m_Extents.m_Size[1],
			m_Extents.m_Size[2]);
#endif

		//

		m_Child[0] = new KdNode(extents1);
		m_Child[1] = new KdNode(extents2);

		m_Child[0]->SplitRecur(map);
		m_Child[1]->SplitRecur(map);
	}

	void SplitUndo()
	{
		Assert(!IsLeaf_get());

		delete m_Child[0];
		delete m_Child[1];

		m_Child[0] = 0;
		m_Child[1] = 0;
	}

	inline BOOL IsLeaf_get() const
	{
		return !m_Child[0];
	}

	ExtentsI m_Extents;
	ExtentsF m_BB;
	PlaneF m_Plane;
	KdNode* m_Child[2];
	BOOL m_IsEmpty;
	BOOL m_IsSolid;
	SphereF m_bbSphere;
};

class KdTree
{
public:
	KdTree()
	{
		m_Root = 0;
	}

	KdNode* m_Root;
};

static KdTree* CreateTree(Map* map)
{
	KdTree* tree = new KdTree();

	tree->m_Root = new KdNode();

	tree->m_Root->m_Extents = ExtentsI(0, 0, 0, MAP_SX, MAP_SY, MAP_SZ);

	tree->m_Root->SplitRecur(map);

	return tree;
}

class IntersectResult
{
public:
	void Initialize(KdNode* node, float distance, VecF normal)
	{
		m_Node = node;
		m_Distance = distance;
		m_Normal = normal;
	}

	KdNode* m_Node;
	float m_Distance;
	VecF m_Normal;
};

class KdNodeStack
{
public:
	KdNodeStack()
	{
		m_StackDepth = 0;
	}

	inline void Reset()
	{
		m_StackDepth = 0;
	}

	inline void Push(KdNode* node)
	{
		m_Nodes[m_StackDepth++] = node;
	}

	inline KdNode* Pop()
	{
		return m_Nodes[--m_StackDepth];
	}

	KdNode* m_Nodes[50];
	int m_StackDepth;
};

typedef void (*IntersectCB)(KdNode* node);

static BOOL Intersect(const KdTree * RESTRICTED tree, VecF pos, VecF dir, IntersectCB cb, IntersectResult * RESTRICTED result)
{
	SimdVec posVec = ToSimdVec(pos);
	SimdVec dirVec = ToSimdVec(dir);
	SimdVec dirInvVec = dirVec.Inv();

	//PlaneF rayPlane;

	//rayPlane.Initialize(dir, dir * pos);

	KdNodeStack stack;

	stack.Push(tree->m_Root);

	class IntersectStats
	{
	public:
		__forceinline IntersectStats()
		{
			Initialize();
		}

#if 1
		__forceinline ~IntersectStats()
		{
			LOG("MaxStackDepth: %d", MaxStackDepth);
			LOG("NodeVisitCount: %d", NodeVisitCount);
		}
#endif

		__forceinline void Initialize()
		{
			MaxStackDepth = -1;
			NodeVisitCount = 0;
		}

		__forceinline void NextStackDepth(int depth)
		{
			if (depth > MaxStackDepth || MaxStackDepth == -1)
				MaxStackDepth = depth;
		}

		__forceinline void NextNodeVisit()
		{
			NodeVisitCount++;
		}

		int MaxStackDepth;
		int NodeVisitCount;
	};

	//IntersectStats stats;

	while (stack.m_StackDepth > 0)
	{
#if 0
		stats.NextStackDepth(stack.m_StackDepth);
		stats.NextNodeVisit();
#endif

		KdNode * RESTRICTED node = stack.Pop();

		Assert(!node->m_IsEmpty);

#if 0
		if (cb)
		{
			cb(node);
		}
#endif

		float tempT;
		VecF tempN;

		// todo: hit BB before pushing nodes.

		// Hit bounding sphere and bounding box before continuing.

		if (!BT_IntersectBox_FastSIMD(posVec, dirInvVec, node->m_BB.m_Min, node->m_BB.m_Max, tempT))
			continue;

#if 0
		LOG("TRY: X=%d, Y=%d, Z=%d, SX=%d, SY=%d, SZ=%d",
			node->m_Extents.m_Min.m_X,
			node->m_Extents.m_Min.m_Y,
			node->m_Extents.m_Min.m_Z,
			node->m_Extents.m_Size.m_X,
			node->m_Extents.m_Size.m_Y,
			node->m_Extents.m_Size.m_Z);
#endif

		if (node->IsLeaf_get())
		{
			result->Initialize(node, tempT, tempN);

			return TRUE;
		}
		else
		{
			const SimdVec d = node->m_Plane * posVec;
			const SimdVec dd = node->m_Plane.m_Normal.Dot3(dirVec);

			if (d.ALL_LE4(SIMD_VEC_ZERO))
			{
				if (dd.ALL_GE4(SIMD_VEC_ZERO))
					stack.Push(node->m_Child[1]);

				stack.Push(node->m_Child[0]);
			}

			if (d.ALL_GE4(SIMD_VEC_ZERO))
			{
				if (dd.ALL_LE4(SIMD_VEC_ZERO))
					stack.Push(node->m_Child[0]);

				stack.Push(node->m_Child[1]);
			}
		}
	}

	return FALSE;
}

BOOL CheckSpace(KdNode* node, Map* map, int* o_IsEmpty, int* o_IsSolid)
{
	*o_IsEmpty = TRUE;
	*o_IsSolid = TRUE;

	for (int x = node->m_Extents.m_Min[0]; x <= node->m_Extents.m_Max[0]; ++x)
	{
		for (int y = node->m_Extents.m_Min[1]; y <= node->m_Extents.m_Max[1]; ++y)
		{
			for (int z = node->m_Extents.m_Min[2]; z <= node->m_Extents.m_Max[2]; ++z)
			{
				int isSolid = map->m_Voxels[x][y][z].IsSolid;

				if (isSolid)
					*o_IsEmpty = FALSE;
				else
					*o_IsSolid = FALSE;
			}
		}
	}

	return TRUE;
}

void Optimize(KdNode* node, Map* map)
{
	if (!node->IsLeaf_get())
	{
		Optimize(node->m_Child[0], map);
		Optimize(node->m_Child[1], map);
	}

	if (node->IsLeaf_get())
		return;

	// If both child nodes are leaf.

	if (node->m_Child[0]->IsLeaf_get() || node->m_Child[1]->IsLeaf_get())
	{
		// And both leafs are empty.

		CheckSpace(node->m_Child[0], map, &node->m_Child[0]->m_IsEmpty, &node->m_Child[0]->m_IsSolid);
		CheckSpace(node->m_Child[1], map, &node->m_Child[1]->m_IsEmpty, &node->m_Child[1]->m_IsSolid);

		int empty = node->m_Child[0]->m_IsEmpty && node->m_Child[1]->m_IsEmpty;
		int solid = node->m_Child[0]->m_IsSolid && node->m_Child[1]->m_IsSolid;

		if (empty)
		{
			node->SplitUndo();
		}
		if (solid)
		{
			//node->SplitUndo();
		}
	}
}

void Optimize(KdTree* tree, Map* map)
{
	Optimize(tree->m_Root, map);
}
