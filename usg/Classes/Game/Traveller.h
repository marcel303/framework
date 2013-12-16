#pragma once

#include "CallBack.h"
#include "Types.h"

namespace Game
{
	typedef struct
	{
		float x;
		float y;
		float dx;
		float dy;
	} TravelEvent;

	class Traveller
	{
	public:
		Traveller();
		void Setup(float step, CallBack travelCB);
		
		void Begin(float x, float y);
		void End(float x, float y);
		
		void Update(float x, float y);
		void TravelCB(float x, float y, float dx, float dy);
		
		bool m_Active;
		Vec2F m_Coord;
		Vec2F m_Direction;
		float m_Travel;
		float m_Step;
		CallBack m_TravelCB;
	};
}
