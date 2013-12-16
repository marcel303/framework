#include "Calc.h"
#include "Traveller.h"

namespace Paint
{
	Traveller::Traveller()
	{
		m_Active = false;
		
		m_Step = 1.0f;
	}

	void Traveller::Update(float x, float y, float pressure)
	{
		if (!m_Active)
			return;
		
		Coord temp;

		temp.p[0] = x;
		temp.p[1] = y;

		Coord delta;

		delta.p[0] = temp.p[0] - m_Coord.p[0];
		delta.p[1] = temp.p[1] - m_Coord.p[1];

		// Discard no-move.
		if (delta.p[0] == 0.0f || delta.p[1] == 0.0f)
			return;

		float distanceSq =
			delta.p[0] * delta.p[0] +
			delta.p[1] * delta.p[1];

		float distance = sqrtf(distanceSq);

		// Update travel state and check for movement.

		m_Travel = distance;

		delta.p[0] /= distance;
		delta.p[1] /= distance;

		while (m_Travel >= m_Step)
		{
			Coord move;

			move.p[0] = delta.p[0] * m_Step;
			move.p[1] = delta.p[1] * m_Step;

			m_Coord.p[0] += move.p[0];
			m_Coord.p[1] += move.p[1];

			m_Direction.p[0] = move.p[0];
			m_Direction.p[1] = move.p[1];

			m_Travel -= m_Step;

			//

			TravelCB(
				m_Coord.p[0],
				m_Coord.p[1],
				m_Direction.p[0],
				m_Direction.p[1],
				pressure);
		}
	}
};
