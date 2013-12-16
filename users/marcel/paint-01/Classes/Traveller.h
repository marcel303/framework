#pragma once

#include "Callback.h"
#include "Coord.h"

// TODO: Do pressure normalization here or in input, but not application.

namespace Paint
{
	typedef struct
	{
		float x;
		float y;
		float dx;
		float dy;
		float pressure;
	} TravelEvent;

	class Traveller
	{
	public:
		Traveller();

		void Begin(int x, int y, float pressure)
		{
			m_Active = true;
			
			m_Coord.p[0] = x;
			m_Coord.p[1] = y;

			TravelCB(x, y, 0.0f, 0.0f, pressure);
		}

		void End(int x, int y)
		{
			m_Active = false;
		}

		void Update(float x, float y, float pressure);

		void TravelCB(float x, float y, float dx, float dy, float pressure)
		{
			TravelEvent e;

			e.x = x;
			e.y = y;
			e.dx = dx;
			e.dy = dy;
			e.pressure = pressure;

			m_TravelCB.Invoke(&e);
		}

		bool m_Active;
		Coord m_Coord;
		Coord m_Direction;
		float m_Travel;
		float m_Step;
		CallBack m_TravelCB;
	};
};