#pragma once

#include "Types.h"

namespace GameUtil
{
	class PathNode
	{
	public:
		void Setup(Vec2F position, float angle);
		void Setup(Vec2F position, Vec2F position2, float angle1, float angle2, float distance1, float distance2);
		
		Vec2F m_Position1;
		Vec2F m_Position2;
		float m_Angle1;
		float m_Angle2;
		float m_Distance1;
		float m_Distance2;
	};
	
	class Path
	{
	public:
		Path();
		
		void PlotNext(Vec2F position, float angle);
		void GetNodes(int count, float intervalDistance, float startDistance, PathNode* out_Nodes);
		bool HasNode(float distance);
		
	private:
		PathNode* Allocate();
		PathNode Interpolate(int index, float t);
		int GetNodeIndexByOffset(int offset) const;
		PathNode* GetNodeByOffset(int offset);
		
		PathNode m_Nodes[256];
		int m_NodeIndex;
		int m_NodeCountMask;
		int m_NodeCount;
	};
};
