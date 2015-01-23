#include "Calc.h"
#include "Debugging.h"
#include "Traveller.h"

Traveller::Traveller()
{
	m_Active = false;
	
	m_Step = 1.0f;
	m_Lag = false;
	m_IsFirst = false;
	
	m_TravelCB = 0;
}

void Traveller::Setup(float step, TravelCB travelCB, void* obj)
{
	m_Step = step;
	m_TravelCB = travelCB;
	m_Obj = obj;
}

void Traveller::Begin(float x, float y)
{
	m_Active = true;
	
	m_Coord[0] = x;
	m_Coord[1] = y;

	m_IsFirst = true;
	
	if (!m_Lag)
	{
		DoTravelCB(x, y, 0.0f, 0.0f);
		
		m_IsFirst = false;
	}
}

void Traveller::End(float x, float y)
{
	Update(x, y);
	
	m_Active = false;
	
/*	if (m_IsFirst)
	{
		DoTravelCB(x, y, 0.0f, 0.0f);
	}*/
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
	if (delta[0] == 0.0f && delta[1] == 0.0f)
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

		if (!m_Lag)
		{
			DoTravelCB(
				m_Coord[0],
				m_Coord[1],
				m_Direction[0],
				m_Direction[1]);
		}
		else
		{
			DoTravelCB(
				m_Coord[0] - move[0],
				m_Coord[1] - move[1],
				m_Direction[0],
				m_Direction[1]);
		}
		
		m_IsFirst = false;
	}
}

void Traveller::DoTravelCB(float x, float y, float dx, float dy)
{
	Assert(m_TravelCB != 0);
	
	TravelEvent e;

	e.x = x;
	e.y = y;
	e.dx = dx;
	e.dy = dy;

	if (m_TravelCB)
		m_TravelCB(m_Obj, e);
}
