#pragma once

#include "klodder_forward.h"
#include "libgg_forward.h"
#include "Types.h"

enum BezierTravellerState
{
	BezierTravellerState_Begin,
	BezierTravellerState_End,
	BezierTravellerState_Update
};

typedef void (*BezierTravelCB)(void* obj, BezierTravellerState state, float x, float y);
typedef void (*BezierTangentCB)(void* obj, const BezierCurve& curve);

class BezierCapturePoint
{
public:
	BezierCapturePoint()
	{
		hasTangent = false;
	}
	BezierCapturePoint(const Vec2F& _position)
	{
		hasTangent = false;
		position = _position;
	}
	
	bool hasTangent;
	Vec2F position;
	Vec2F tangent;
};

class BezierTraveller
{
public:
	BezierTraveller();
	void Setup(float maxStep, BezierTravelCB travelCB, BezierTangentCB tangentCB, void* obj);
	
	void Begin(float x, float y);
	void End(float x, float y);
	void Update(float x, float y);
	
	Vec2F LastLocation_get() const;

private:
	void EmitLeft() const;
	void EmitRight() const;
	void EmitStraight() const;
	void Sample(const BezierCurve& curve) const;
	
	void AddPoint(const Vec2F& point);
	int GetPointIndex(int index) const;
	BezierCapturePoint& GetPoint(int index);
	const BezierCapturePoint& GetPoint(int index) const;
	
	int mHistorySize;
	int mHistoryCursor;
	BezierCapturePoint mHistory[3];
	
	Vec2F mLastLocation;
	
	float mMaxStep;
	BezierTravelCB mTravelCB;
	BezierTangentCB mTangentCB;
	void* mObj;
};
