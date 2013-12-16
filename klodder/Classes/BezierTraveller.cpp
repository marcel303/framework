#include "Bezier.h"
#include "BezierTraveller.h"
#include "Calc.h"

#define FACTOR 0.25f

static void MakeCurve(const BezierCapturePoint& p1, const BezierCapturePoint& p2, BezierCurve& out_Curve);

BezierTraveller::BezierTraveller()
{
	mMaxStep = 1.0f;
	mTravelCB = 0;
	mTangentCB = 0;
	mObj = 0;

	mHistoryCursor = 0;
	mHistorySize = 0;
}

void BezierTraveller::Setup(float maxStep, BezierTravelCB travelCB, BezierTangentCB tangentCB, void* obj)
{
	mMaxStep = maxStep;
	mTravelCB = travelCB;
	mTangentCB = tangentCB;
	mObj = obj;
}

void BezierTraveller::Begin(float x, float y)
{
	mLastLocation.Set(x, y);
	
	mTravelCB(mObj, BezierTravellerState_Begin, x, y);
	
	AddPoint(Vec2F(x, y));
}

void BezierTraveller::End(float x, float y)
{
	if (x != mLastLocation[0] || y != mLastLocation[1])
	{
		mLastLocation.Set(x, y);
		
		AddPoint(Vec2F(x, y));
	
		EmitLeft();
	}
	
	//

#ifdef DEBUG
#if 0
	if (mHistorySize == 2)
	{
		EmitStraight();
	}
#endif
#endif

	if (mHistorySize == 3)
	{
		EmitRight();
	}
	
	//
	
	mTravelCB(mObj, BezierTravellerState_End, x, y);
	
	//
	
	mHistorySize = 0;
	
	for (int i = 0; i < 3; ++i)
		mHistory[i].hasTangent = false;
}

void BezierTraveller::Update(float x, float y)
{
	if (x != mLastLocation[0] || y != mLastLocation[1])
	{
		mLastLocation.Set(x, y);
	
		AddPoint(Vec2F(x, y));
	
		EmitLeft();
	}
}

Vec2F BezierTraveller::LastLocation_get() const
{
	return mLastLocation;
}

void BezierTraveller::EmitLeft() const
{
	if (mHistorySize == 3)
	{
		BezierCurve curve;
		
		const BezierCapturePoint& p1 = GetPoint(0);
		const BezierCapturePoint& p2 = GetPoint(1);
		
		MakeCurve(p1, p2, curve);
		
		Sample(curve);
	}
}

void BezierTraveller::EmitRight() const
{
	if (mHistorySize == 3)
	{
		BezierCurve curve;
		
		const BezierCapturePoint& p1 = GetPoint(1);
		const BezierCapturePoint& p2 = GetPoint(2);
		
		MakeCurve(p1, p2, curve);
		
		Sample(curve);
	}
}

void BezierTraveller::EmitStraight() const
{
	if (mHistorySize == 2)
	{
		BezierCurve curve;
		
		const BezierCapturePoint& p1 = GetPoint(0);
		const BezierCapturePoint& p2 = GetPoint(1);
		
		MakeCurve(p1, p2, curve);
		
		Sample(curve);
	}
}

class SampleState
{
public:
	inline SampleState(const BezierCurve& _curve) : curve(_curve)
	{
	}
	
	const BezierCurve& curve;
	float maxDistanceSq;
	BezierTravellerState state;
	BezierTravelCB travelCB;
	void* obj;
};

static void Sample(const SampleState& state, float t1, float t2)
{
	const Vec2F p1 = state.curve.Interpolate(t1);
	const Vec2F p2 = state.curve.Interpolate(t2);
	
	const float distanceSq = p1.DistanceSq(p2);
	
	if (distanceSq > state.maxDistanceSq)
	{
		const float tMid = (t1 + t2) * 0.5f;

		Sample(state, t1, tMid);
		Sample(state, tMid, t2);
	}
	else
	{
		state.travelCB(
			state.obj,
			state.state,
			(p1[0] + p2[0]) * 0.5f,
			(p1[1] + p2[1]) * 0.5f);
	}
}

void BezierTraveller::Sample(const BezierCurve& curve) const
{
	Assert(mTravelCB != 0);
	
	SampleState state(curve);
	state.maxDistanceSq = mMaxStep * mMaxStep;
	state.state = BezierTravellerState_Update;
	state.travelCB = mTravelCB;
	state.obj = mObj;
	
	::Sample(state, 0.0f, 1.0f);

	if (mTangentCB)
		mTangentCB(mObj, curve);
}

void BezierTraveller::AddPoint(const Vec2F& point)
{
	mHistory[mHistoryCursor] = BezierCapturePoint(point);
	
	mHistoryCursor = mHistoryCursor + 1;
	if (mHistoryCursor == 3)
		mHistoryCursor = 0;
	if (mHistorySize < 3)
		mHistorySize++;
	
	Assert(mHistoryCursor >= 0 && mHistoryCursor <= 2);
	Assert(mHistory >= 0 && mHistorySize <= 3);
	
	if (mHistorySize == 2)
	{
		BezierCapturePoint& p1 = GetPoint(0);
		const BezierCapturePoint& p2 = GetPoint(1);
		
		Vec2F delta = p2.position - p1.position;
		p1.tangent = delta.Normal();
		p1.hasTangent = true;
	}
	
	if (mHistorySize == 3)
	{
		// calculate tangent for mid-point
		
		const BezierCapturePoint& p1 = GetPoint(0);
		BezierCapturePoint& p2 = GetPoint(1);
		const BezierCapturePoint& p3 = GetPoint(2);
		
#if 0
		p2.tangent = p3.position - p1.position;
		p2.hasTangent = true;
#else
		const Vec2F delta1 = (p3.position - p2.position).Normal();
		const Vec2F delta2 = (p1.position - p2.position).Normal();
		Assert(delta1.Length_get() > 0.0f);
		Assert(delta2.Length_get() > 0.0f);
		
		const Vec2F direction1 = delta1.Normal();
		const Vec2F direction2 = delta2.Normal();
		
		//Vec2F delta = (delta1 + delta2).Normal();
		Vec2F delta = direction1 / delta1.Length_get() + direction2 / delta2.Length_get();
//		Assert(delta.Length_get() > 0.0f);
		Vec2F tangent;
		if (delta.LengthSq_get() > 0)
			tangent = Vec2F::FromAngle(delta.ToAngle() + Calc::mPI2);
		if (tangent * p2.position - tangent * p3.position < 0.0f)
			p2.tangent = tangent;
		else
			p2.tangent = -tangent;
		p2.hasTangent = true;
#endif
	}
}

int BezierTraveller::GetPointIndex(int index) const
{
	return (mHistoryCursor + 3 - mHistorySize + index) % 3;
}

BezierCapturePoint& BezierTraveller::GetPoint(int index)
{
	return mHistory[GetPointIndex(index)];
}

const BezierCapturePoint& BezierTraveller::GetPoint(int index) const
{
	return mHistory[GetPointIndex(index)];
}

static void MakeCurve(
	const BezierCapturePoint& p1,
	const BezierCapturePoint& p2,
	BezierCurve& out_Curve)
{	
#if 1
//	const float factor = 1.0f;
	const float factor = 0.5f;
	//const float factor = 0.4f;
//	const float factor = 0.3f;
	//const float factor = 0.2f;
	
	const Vec2F delta = p2.position - p1.position;
	//const float distance = delta.Length_get();
	const float distance1 = fabsf(p1.tangent * p1.position - p1.tangent * p2.position);
	const float distance2 = fabsf(p2.tangent * p2.position - p2.tangent * p1.position);
	
	Vec2F points[4];
	
	points[0] = p1.position;
	points[1] = p1.hasTangent ? p1.tangent : delta;
	points[2] = p2.hasTangent ? -p2.tangent : -delta;
	points[3] = p2.position;
	
	points[1] = points[1].Normal() * distance1 * factor;
	points[2] = points[2].Normal() * distance2 * factor;
	
	out_Curve.Setup_RelativeTangents(points);
#else
	const float factor = 0.2f;
	
	const Vec2F delta = p2.position - p1.position;
	
	Vec2F points[4];
	
	points[0] = p1.position;
	points[1] = p1.hasTangent ? p1.tangent : delta;
	points[2] = p2.hasTangent ? -p2.tangent : -delta;
	points[3] = p2.position;
	
	points[1] *= factor;
	points[2] *= factor;
	
	out_Curve.Setup_RelativeTangents(points);
#endif
}
