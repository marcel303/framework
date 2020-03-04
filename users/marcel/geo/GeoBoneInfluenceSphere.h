#pragma once

class ProcInfluenceSphere : public ProcInfluence
{
public:
	ProcInfluenceSphere() : ProcInfluence()
	{
		m_radius = 1.0f;
		m_falloff1 = 1.0f;
		m_falloff2 = 1.0f;
	}
	virtual float CalculateInfluence(const Vector& position)
	{
		Vector delta = position - m_position;
		float distance = delta.GetSize() * 100.0f;
		//return 1.0f / pow((distance + 1.0f), 4.0f);

		if (distance < m_radius)
			return 1.0f;
		distance -= m_radius;
		float influence1 = 1.0f / (m_falloff1 * distance + 1.0f);
		float influence2 = 1.0f / (m_falloff2 * distance + 1.0f) * (m_falloff1 * distance + 1.0f);
		return influence1 * influence2;
	}

	Vector m_position;
	float m_radius;
	float m_falloff1;
	float m_falloff2;
};
