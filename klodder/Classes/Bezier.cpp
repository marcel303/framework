#include "Bezier.h"

void BezierCurve::Setup_RelativeTangents(const Vec2F * p)
{
	mPoints[0] = p[0];
	mPoints[1] = p[0] + p[1];
	mPoints[2] = p[3] + p[2];
	mPoints[3] = p[3];
}

Vec2F BezierCurve::Interpolate(const float t) const
{
	const Vec2F * points = mPoints;
	
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
	mCurveCount = 0;
}

void BezierPath::Allocate(const int curveCount)
{
	mCurves.clear();
	mCurveCount = 0;
	
	if (curveCount > 0)
	{
		mCurves.resize(curveCount);
		for (int i = 0; i < curveCount; ++i)
			mCurves[i] = BezierCurve();
		mCurveCount = curveCount;
	}
}

void BezierPath::ConstructFromNodes(const BezierNode * nodes, const int nodeCount)
{
	const int curveCount = nodeCount - 1;
	
	Allocate(curveCount);
	
	for (int i = 0; i < curveCount; ++i)
	{
		const BezierNode & node1 = nodes[i + 0];
		const BezierNode & node2 = nodes[i + 1];
		
		mCurves[i].mPoints[0] = node1.mPosition;
		mCurves[i].mPoints[1] = node1.mPosition + node1.mTangent[1];
		mCurves[i].mPoints[2] = node2.mPosition + node2.mTangent[0];
		mCurves[i].mPoints[3] = node2.mPosition;
	}
}

Vec2F BezierPath::Interpolate(const float _t) const
{
	int curveIndex = (int)floorf(_t);
	
	if (curveIndex >= mCurveCount)
		curveIndex = mCurveCount - 1;
	
	float t = _t - curveIndex;
	
	if (t > 1.0f)
		t = 1.0f;
	
	return mCurves[curveIndex].Interpolate(t);
}

std::vector<Vec2F> BezierPath::Sample(const int resolution, const bool exclusive) const
{
	std::vector<Vec2F> result;
	
	float step;
	
	if (resolution < 2)
		step = 0.0f;
	else
		step = exclusive ? 1.0f / resolution : 1.0f / (resolution - 1);
	
	float t = 0.0f;
	
	result.resize(resolution);
	
	for (int i = 0; i < resolution; ++i, t += step)
	{
		result[i] = Interpolate(t);
	}
	
	return result;
}
