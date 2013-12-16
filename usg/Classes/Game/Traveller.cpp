#include "Calc.h"
#include "Debugging.h"
#include "Traveller.h"

namespace Game
{
	Traveller::Traveller()
	{
		m_Active = false;
		
		m_Step = 1.0f;
	}
	
	void Traveller::Setup(float step, CallBack travelCB)
	{
		m_Step = step;
		m_TravelCB = travelCB;
	}
	
	void Traveller::Begin(float x, float y)
	{
		m_Active = true;
		
		m_Coord[0] = x;
		m_Coord[1] = y;

		TravelCB(x, y, 0.0f, 0.0f);
	}

	void Traveller::End(float x, float y)
	{
		m_Active = false;
	}

	void Traveller::Update(float x, float y)
	{
		if (!m_Active)
			return;
		
		Vec2F temp;

		temp[0] = x;
		temp[1] = y;

		Vec2F delta;

		delta[0] = temp[0] - m_Coord[0];
		delta[1] = temp[1] - m_Coord[1];

		// Discard no-move.
		if (delta[0] == 0.0f || delta[1] == 0.0f)
			return;

		float distanceSq =
			delta[0] * delta[0] +
			delta[1] * delta[1];

		float distance = sqrtf(distanceSq);

		// Update travel state and check for movement.

		m_Travel = distance;

		delta[0] /= distance;
		delta[1] /= distance;

		while (m_Travel >= m_Step)
		{
			Vec2F move;

			move[0] = delta[0] * m_Step;
			move[1] = delta[1] * m_Step;

			m_Coord[0] += move[0];
			m_Coord[1] += move[1];

			m_Direction[0] = move[0];
			m_Direction[1] = move[1];

			m_Travel -= m_Step;

			//

			TravelCB(
				m_Coord[0],
				m_Coord[1],
				m_Direction[0],
				m_Direction[1]);
		}
	}
	
	void Traveller::TravelCB(float x, float y, float dx, float dy)
	{
		Assert(m_TravelCB.IsSet());
		
		TravelEvent e;

		e.x = x;
		e.y = y;
		e.dx = dx;
		e.dy = dy;

		m_TravelCB.Invoke(&e);
	}
}
