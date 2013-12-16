#pragma once

#include "Types.h"

#define MAX_TOUCHES 10

class TouchInfo
{
public:
	TouchInfo();
	
	int m_TouchCount;
	int m_FingerIndex;
	Vec2F m_Location;
	Vec2F m_LocationDelta;
	Vec2F m_LocationView;
	Vec2F m_LocationWithOffset;
	double m_TapTime;
	double m_TapInterval;
};
