#pragma once

static int CountLeafs(KdTree* tree)
{
	int result = 0;

	KdNodeStack stack;

	stack.Push(tree->m_Root);

	while (stack.m_StackDepth > 0)
	{
		KdNode* node = stack.Pop();

		if (node->IsLeaf_get())
		{
			result++;

			continue;
		}

		stack.Push(node->m_Child[0]);
		stack.Push(node->m_Child[1]);
	}

	return result;
}

static KdNode* Locate(KdTree* tree, SimdVecArg pos)
{
	KdNode* node = tree->m_Root;

	while (!node->IsLeaf_get())
	{
		//VecI posI(pos[0], pos[1], pos[2]);

		SimdVec distance = node->m_Plane * pos;

		if (distance.ALL_LE4(SIMD_VEC_ZERO))
			node = node->m_Child[0];
		else
			node = node->m_Child[1];
	}

	return node;
}

static int CountNodes(KdTree* tree)
{
	int result = 0;

	KdNodeStack stack;

	stack.Push(tree->m_Root);

	while (stack.m_StackDepth > 0)
	{
		KdNode* node = stack.Pop();

		result++;

		if (node->IsLeaf_get())
			continue;

		stack.Push(node->m_Child[0]);
		stack.Push(node->m_Child[1]);
	}

	return result;
}

static int CountFills(KdTree* tree)
{
	int result = 0;

	KdNodeStack stack;

	stack.Push(tree->m_Root);

	while (stack.m_StackDepth > 0)
	{
		KdNode* node = stack.Pop();

		if (node->IsLeaf_get())
		{
			if (!node->m_IsEmpty)
				result++;

			continue;
		}

		stack.Push(node->m_Child[0]);
		stack.Push(node->m_Child[1]);
	}

	return result;
}

static void PrintNodes(KdTree* tree)
{
	KdNodeStack stack;

	stack.Push(tree->m_Root);

	while (stack.m_StackDepth > 0)
	{
		KdNode* node = stack.Pop();

		if (!node->IsLeaf_get())
		{
			stack.Push(node->m_Child[0]);
			stack.Push(node->m_Child[1]);
		}

		if (!node->IsLeaf_get())
			continue;

		LOG("Node: Min: %d %d %d",
			node->m_Extents.m_Min[0],
			node->m_Extents.m_Min[1],
			node->m_Extents.m_Min[2]);
		LOG("Node: Max: %d %d %d",
			node->m_Extents.m_Max[0],
			node->m_Extents.m_Max[1],
			node->m_Extents.m_Max[2]);
		LOG("Node: Empty: %d", (int)node->m_IsEmpty);

#if 1
		// For unoptimized tree, all leaf nodes must be sized 1x1x1.

		Assert(node->m_Extents.Size_get()[0] == 1);
		Assert(node->m_Extents.Size_get()[1] == 1);
		Assert(node->m_Extents.Size_get()[2] == 1);
#endif
	}
}

static void DrawNode(KdNode* node)
{
	int v = 40;

	int colRect;
	int colFill;

	if (node->IsLeaf_get())
	{
		colRect = makecol(v, v, v);

		if (node->m_IsEmpty)
			colFill = makecol(0, 0, 0);
		else
			colFill = makecol(0, v / 2, 0);
	}
	else
	{
		colFill = makecol(0, 0, 0);
		colRect = makecol(v, 0, 0);
	}

	VecI size = node->m_Extents.Size_get();

	Draw_Rect(
		node->m_Extents.m_Min[0],
		node->m_Extents.m_Min[1],
		node->m_Extents.m_Min[0] + size[0],
		node->m_Extents.m_Max[1] + size[1],
		colFill);

	Draw_Rect(
		node->m_Extents.m_Min[0],
		node->m_Extents.m_Min[1],
		node->m_Extents.m_Min[0] + size[0],
		node->m_Extents.m_Max[1] + size[1],
		colRect);

#if 1
	VecI mid = node->m_Extents.m_Min + size / 2;

	VecF p1 = VecF(mid[0], mid[1], mid[2]);
	VecF p2 = p1 + ToVecF(node->m_Plane.m_Normal);

	DrawLine(p1[0], p1[1], p2[0], p2[1]);
#endif
}

static void DrawNodes(KdTree* tree, float scale)
{
	KdNodeStack stack;

	stack.Push(tree->m_Root);

	while (stack.m_StackDepth > 0)
	{
		KdNode* node = stack.Pop();

		if (!node->IsLeaf_get())
		{
			stack.Push(node->m_Child[0]);
			stack.Push(node->m_Child[1]);
		}

		if (!node->IsLeaf_get())
			continue;

		DrawNode(node);

		//rest(10);
	}
}

static void PrintStats(KdTree* tree)
{
	int nodeCount = CountNodes(tree);
	int leafCount = CountLeafs(tree);
	int fillCount = CountFills(tree);

	LOG("NodeCount: %d", nodeCount);
	LOG("LeafCount: %d", leafCount);
	LOG("FillCount: %d", fillCount);
}
