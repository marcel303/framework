#pragma once

#include <cmath>

class Thingy
{
public:
	Thingy();
	
	void Setup(float x, float y, float px, float py, float pd, float size, bool isInteractive);
	
	void MakeAngle(float angle);
	
	void Render();
	
	inline float DistanceTo(float x, float y) const
	{
		float dx = x - mPosX;
		float dy = y - mPosY;
		
		return std::sqrt(dx * dx + dy * dy);
	}
	
	inline float PlaneDistanceTo(float x, float y) const
	{
		return x * mPlaneX + y * mPlaneY - mPlaneD;
	}
	
	bool mIsActive;
	float mPosX;
	float mPosY;
	float mPlaneX;
	float mPlaneY;
	float mPlaneD;
	float mSize;
	bool mIsInteractive;
};
