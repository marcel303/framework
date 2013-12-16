#include "Calc.h"
#include "Debugging.h"
#include "Log.h"
#include "Util_Path.h"

namespace GameUtil
{
	void PathNode::Setup(Vec2F position, float angle)
	{
		Setup(position, position, angle, angle, 0.0f, 0.0f);
	}
	
	void PathNode::Setup(Vec2F position1, Vec2F position2, float angle1, float angle2, float distance1, float distance2)
	{
		m_Position1 = position1;
		m_Position2 = position2;
		m_Angle1 = angle1;
		m_Angle2 = angle2;
		m_Distance1 = distance1;
		m_Distance2 = distance2;
	}
	
	//
	
	Path::Path()
	{
		m_NodeIndex = -1;
		m_NodeCountMask = 255;
		m_NodeCount = 0;
		
		for (int i = 0; i < 256; ++i)
		{
			Vec2F inf(1000000.0f, 1000000.0f);
			
			m_Nodes[i].Setup(inf, inf, 0.0f, 0.0f, 0.0f, 0.0f);
		}
	}
	
	void Path::PlotNext(Vec2F position, float angle)
	{
		PathNode* prev = GetNodeByOffset(0);
		PathNode* node = Allocate();
		
		Vec2F position1 = position;
		Vec2F position2 = position;
		
		float angle1 = angle;
		float angle2 = angle;
		
		float distance1 = 0.0f;
		
		if (prev)
		{
			position1 = prev->m_Position2;
			angle1 = prev->m_Angle2;
			distance1 = prev->m_Distance2;
		}
		
		Vec2F delta = position2 - position1;
		float distance = delta.Length_get();
		
		float distance2 = distance1 + distance;
		
//		LOG(LogLevel_Debug, "path node: p1=(%f, %f), p2=(%f, %f), delta=(%f, %f), d1=%f, d2=%f", position1[0], position1[1], position2[0], position2[1], delta[0], delta[1], distance1, distance2);
		
		node->Setup(position1, position2, angle1, angle2, distance1, distance2);
		
		//

		m_NodeCount++;
		
		if (m_NodeCount > m_NodeCountMask)
			m_NodeCount = m_NodeCountMask;
	}
	
	void Path::GetNodes(int count, float intervalDistance, float startDistance, PathNode* out_Nodes)
	{
		float distance = startDistance;

		int offset = 0;
		
		PathNode* node = GetNodeByOffset(offset);
		
		for (int i = 0; i < count; ++i) 
		{
			while (distance < node->m_Distance1 && -offset < m_NodeCount - 1)
			{
				offset--;
				
				node = GetNodeByOffset(offset);
			}
			
			float dd = node->m_Distance2 - node->m_Distance1;
			
			Assert(dd >= 0.0f - 0.0001f);
			
			float t = 1.0f;
			
			if (dd > 0.0f)
				t =  (distance - node->m_Distance1) / dd;
			
			if (distance <= 0.0f)
				t = 0.0f;
			
			Assert(t >= 0.0f - 0.0001f);
			Assert(t <= 1.0f + 0.0001f);
			
#if 1
			if (t < 0.0f)
				t = 0.0f;
			if (t > 1.0f)
				t = 1.0f;
#endif
			
			out_Nodes[i] = Interpolate(offset, t);
			
			distance -= intervalDistance;
		}
	}
	
	bool Path::HasNode(float distance)
	{
		const PathNode* last = GetNodeByOffset(0);
		
		if (!last)
			return false;
		
		if (distance > last->m_Distance2)
			return false;
		
		return true;
	}
	
	PathNode* Path::Allocate()
	{
		m_NodeIndex = (m_NodeIndex + 1) & 255;
		
		PathNode* node = &m_Nodes[m_NodeIndex];
		
		return node;
	}
	
	PathNode Path::Interpolate(int offset, float t)
	{
		PathNode* node = GetNodeByOffset(offset);
		
		PathNode result;
		
		// interpolate angle
		
		float angleDiff = node->m_Angle2 - node->m_Angle1;
		
		if (angleDiff > +Calc::mPI)
			angleDiff = angleDiff - Calc::mPI * 2.0f;
		if (angleDiff < -Calc::mPI)
			angleDiff = angleDiff + Calc::mPI * 2.0f;
		
		float angle = node->m_Angle1 + angleDiff * t;
		
		result.Setup(node->m_Position1.LerpTo(node->m_Position2, t), angle);
		
		return result;
	}
	
	int Path::GetNodeIndexByOffset(int offset) const
	{
		return (m_NodeIndex + offset) & 255;
	}
	
	PathNode* Path::GetNodeByOffset(int offset)
	{
		if (m_NodeCount == 0)
			return 0;
		
		const int index = GetNodeIndexByOffset(offset);
		
		return &m_Nodes[index];
	}
};
