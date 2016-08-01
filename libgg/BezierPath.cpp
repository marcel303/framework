#include "BezierPath.h"
#include "Calc.h"

void BezierCurve::Setup_RelativeTangents(const Vec2F* p)
{
	m_Points[0] = p[0];
	m_Points[1] = p[0] + p[1];
	m_Points[2] = p[3] + p[2];
	m_Points[3] = p[3];
}

Vec2F BezierCurve::Interpolate(float t) const
{
	const Vec2F* points = m_Points;
	
	const float t1 = 1.0f - t;
	const float t2 = t;

	const float x =
		1.0f * points[0].x * t1 * t1 * t1 +
		3.0f * points[1].x * t1 * t1 * t2 +
		3.0f * points[2].x * t2 * t2 * t1 +
		1.0f * points[3].x * t2 * t2 * t2;

	const float y =
		1.0f * points[0].y * t1 * t1 * t1 +
		3.0f * points[1].y * t1 * t1 * t2 +
		3.0f * points[2].y * t2 * t2 * t1 +
		1.0f * points[3].y * t2 * t2 * t2;

	return Vec2F(x, y);
}

BezierPath::BezierPath()
{
	Initialize();
}

BezierPath::~BezierPath()
{
	Allocate(0);
}

void BezierPath::Initialize()
{
	m_CurveCount = 0;
}

void BezierPath::Allocate(int curveCount)
{
	m_Curves.clear();
	m_CurveCount = 0;
	
	if (curveCount > 0)
	{
		m_Curves.resize(curveCount);
		m_CurveCount = curveCount;
	}
}

void BezierPath::ConstructFromNodes(BezierNode* nodes, int nodeCount)
{
	int curveCount = nodeCount - 1;
	
	Allocate(curveCount);
	
	for (int i = 0; i < curveCount; ++i)
	{
		BezierNode& node1 = nodes[i + 0];
		BezierNode& node2 = nodes[i + 1];
		
		m_Curves[i].m_Points[0] = node1.m_Position;
		m_Curves[i].m_Points[1] = node1.m_Position + node1.m_Tangent[1];
		m_Curves[i].m_Points[2] = node2.m_Position + node2.m_Tangent[0];
		m_Curves[i].m_Points[3] = node2.m_Position;
	}
}

Vec2F BezierPath::Interpolate(float t) const
{
	int curveIndex = (int)floorf(t);
	
	if (curveIndex >= m_CurveCount)
		curveIndex = m_CurveCount - 1;
	
	t = t - curveIndex;
	
	if (t > 1.0f)
		t = 1.0f;
	
	return m_Curves[curveIndex].Interpolate(t);
}

//

BezierAnim::BezierAnim()
{
	Initialize();
}

void BezierAnim::Initialize()
{
}

void BezierAnim::Setup(TimeTracker* timeTracker, float duration, AnimTimerRepeat repeat, const BezierPath& path, TweenType tweenType)
{
	m_Path = path;
	m_Anim.Initialize(timeTracker, false);
	m_Anim.Start(AnimTimerMode_TimeBased, false, duration, repeat);
	m_Tween.Setup(tweenType, tweenType);
}

void BezierAnim::Start()
{
	m_Anim.Restart();
}

float BezierAnim::Progress_get()
{
	return m_Anim.Progress_get();
}

void BezierAnim::Progress_set(float progress)
{
#ifndef DEPLOYMENT
	throw ExceptionVA("not yet implemented");
#endif
	//m_Anim.Progress_set(progress);
}

bool BezierAnim::IsRunning_get()
{
	return m_Anim.IsRunning_get();
}

Vec2F BezierAnim::Position_get()
{
	float t = m_Anim.Progress_get();
	
	return CalcPosition(t);
}

Vec2F BezierAnim::Direction_get()
{
	float t1 = m_Anim.Progress_get() - 0.001f;
	float t2 = m_Anim.Progress_get() + 0.001f;
	
	Vec2F p1 = CalcPosition(t1);
	Vec2F p2 = CalcPosition(t2);
	
	Vec2F dir = p2 - p1;
	
	dir.Normalize();
	
	return dir;
}

const BezierPath& BezierAnim::Path_get() const
{
	return m_Path;
}

Vec2F BezierAnim::CalcPosition(float t) const
{
	t = m_Tween.Tween(t) * m_Path.m_CurveCount;
	
	return m_Path.Interpolate(t);
}

//

TweenSegment::TweenSegment()
{
	Initialize();
}

void TweenSegment::Initialize()
{
	m_BeginType = TweenType_Undefined;
	m_EndType = TweenType_Undefined;
}

void TweenSegment::Setup(TweenType begin, TweenType end)
{
	m_BeginType = begin;
	m_EndType = end;
}

/*
static float Interpolate_Linear(float v1, float v2, float t)
{
	return v1 + (v2 - v1) * t;
}

static float Interpolate_Cosine(float v1, float v2, float t)
{
	return v2 - (v2 - v1) * (cosf(t * Calc::mPI) + 1.0f) * 0.5f;
}
*/

float TweenSegment::Tween(float t) const
{
#if 0
	int seg = t < 0.5f ? 0 : 1;
	
	TweenType type = TweenType_Undefined;
	float v1 = 0.0f;
	float v2 = 1.0f;
	
	if (seg == 0)
	{
		v1 = 0.0f;
		v2 = 1.0f;
		type = m_BeginType;
	}
	else
	{
		t = (t - 0.5f) * 2.0f;
		v1 = 0.5f;
		v2 = 1.0f;
		type = m_EndType;
	}
	
	switch (type)
	{
		case TweenType_Linear:
			return Interpolate_Linear(v1, v2, t);
		case TweenType_Cosine:
			return Interpolate_Cosine(v1, v2, t);
		default:
#ifndef DEPLOYMENT
			throw ExceptionNA();
#else
			return Interpolate_Linear(v1, v2, t);
#endif
	}
#else
	return t;
#endif
}
