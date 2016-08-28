#pragma once

#include <vector>
#include "Types.h"

class BezierNode
{
public:
	BezierNode()
	{
	}
	
	BezierNode(const Vec2F & position, const Vec2F & tangent1, const Vec2F & tangent2)
	{
		mPosition = position;
		mTangent[0] = tangent1;
		mTangent[1] = tangent2;
	}
	
	Vec2F mPosition;
	Vec2F mTangent[2];
};

class BezierCurve
{
public:
	void Setup_RelativeTangents(const Vec2F * p);
	Vec2F Interpolate(const float t) const;
	
	// position1, tangent1, tangent2, position2
	Vec2F mPoints[4];
};

class BezierPath
{
public:
	BezierPath();
	~BezierPath();
	void Initialize();
	
	void Allocate(const int curveCount);
	void ConstructFromNodes(const BezierNode * nodes, const int nodeCount);
	Vec2F Interpolate(const float t) const;
	std::vector<Vec2F> Sample(const int resolution, const bool exclusive) const;
	
	std::vector<BezierCurve> mCurves;
	int mCurveCount;
};
