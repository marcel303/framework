#include "Calc.h"
#include "render.h"
#include "Thingy.h"
#include "Types.h"

Thingy::Thingy()
{
	mIsActive = false;
}

void Thingy::Setup(float x, float y, float px, float py, float pd, float size, bool isInteractive)
{
	mIsActive = true;
	
	mPosX = x;
	mPosY = y;
	mPlaneX = px;
	mPlaneY = py;
	mPlaneD = pd;
	mSize = size;
	mIsInteractive = isInteractive;
}

void Thingy::MakeAngle(float angle)
{
	Vec2F normal = Vec2F::FromAngle(angle + Calc::mPI2);
	
	mPlaneX = normal[0];
	mPlaneY = normal[1];
	mPlaneD = normal[0] * mPosX + normal[1] * mPosY;
}

void Thingy::Render()
{
	if (!mIsInteractive)
		return;
	
	float angle = Vec2F(mPlaneX, mPlaneY).ToAngle() + Calc::mPI2;
	Vec2F dir = Vec2F::FromAngle(angle);
	Vec2F p1 = Vec2F(mPosX, mPosY) - dir * mSize;
	Vec2F p2 = Vec2F(mPosX, mPosY) + dir * mSize;
	
	gRender->Line(p1[0], p1[1], p2[0], p2[1], Color(0.0f, 1.0f, 0.0f));
}
