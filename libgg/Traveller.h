#pragma once

#include "Types.h"

class TravelEvent
{
public:
	float x;
	float y;
	float dx;
	float dy;
};

typedef void (*TravelCB)(void* obj, const TravelEvent& e);

class Traveller
{
public:
	Traveller();
	void Setup(float step, TravelCB travelCB, void* obj);
	
	void Begin(float x, float y);
	void End(float x, float y);
	
	void Update(float x, float y);
	void DoTravelCB(float x, float y, float dx, float dy);
	
	bool m_Active;
	Vec2F m_Coord;
	Vec2F m_Direction;
	float m_Travel;
	float m_Step;
	bool m_Lag;
	bool m_IsFirst;
	TravelCB m_TravelCB;
	void* m_Obj;
};
