#pragma once

#include <vector>
#include "AnimTimer.h"
#include "Types.h"

class BezierNode
{
public:
	inline BezierNode()
	{
	}
	
	inline BezierNode(Vec2F position, Vec2F tangent1, Vec2F tangent2)
	{
		m_Position = position;
		m_Tangent[0] = tangent1;
		m_Tangent[1] = tangent2;
	}
	
	Vec2F m_Position;
	Vec2F m_Tangent[2];
};

class BezierCurve
{
public:
	void Setup_RelativeTangents(const Vec2F* p);
	Vec2F Interpolate(float t) const;
	
	// position1, tangent1, tangent2, position2
	Vec2F m_Points[4];
};


class BezierPath
{
public:
	BezierPath();
	~BezierPath();
	void Initialize();
	
	void Allocate(int curveCount);
	void ConstructFromNodes(BezierNode* nodes, int nodeCount);
	Vec2F Interpolate(float t) const;
	
	std::vector<BezierCurve> m_Curves;
	int m_CurveCount;
};

enum TweenType
{
	TweenType_Undefined,
	TweenType_Linear,
	TweenType_Cosine,
};

class TweenSegment
{
public:
	TweenSegment();
	void Initialize();
	
	void Setup(TweenType begin, TweenType end);
	
	float Tween(float t) const;
	
private:
	TweenType m_BeginType;
	TweenType m_EndType;
};

class BezierAnim
{
public:
	BezierAnim();
	void Initialize();
	
	void Setup(TimeTracker* timeTracker, float duration, AnimTimerRepeat repeat, const BezierPath& path, TweenType tweenType);
	
	void Start();
	
	float Progress_get();
	void Progress_set(float progress);
	bool IsRunning_get();
	
	Vec2F Position_get();
	Vec2F Direction_get();
	
	const BezierPath& Path_get() const;
	
private:
	Vec2F CalcPosition(float t) const;
	
	BezierPath m_Path;
	TweenSegment m_Tween;
	AnimTimer m_Anim;
};
