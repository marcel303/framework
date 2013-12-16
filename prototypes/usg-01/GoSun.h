#pragma once

#include "IObject.h"
#include "MaskMap.h"
#include "PowerSink.h"
#include "Types.h"

class SunPart
{
public:
	SunPart()
	{
		m_Pos = Vec2F(0, 0);
		m_IsDead = FALSE;
		m_HitPoints = 1000;
		m_Mask = 0;
	}

	void Hit(int hitPoints)
	{
		m_HitPoints -= hitPoints;

		if (m_HitPoints < 0)
		{
			m_HitPoints = 0;
			m_IsDead = TRUE;
		}
	}

	Vec2F m_Pos;
	BOOL m_IsDead;
	int m_HitPoints;
	MaskMap* m_Mask;
};

#define SUN_PART_COUNT 4

class Sun : public IObject // Sun / energy core.
{
public:
	Sun(Map* map);

	void Setup(Vec2F pos);

	BOOL GetPartAngles(int index, float& o_Angle1, float& o_Angle2);
	SunPart* GetPart(Vec2F pos);

	void Hit(Vec2F pos, int hitPoints);

	virtual void Update(Map* map);
	virtual void Render(BITMAP* buffer);
	virtual void RenderSB(SelectionBuffer* sb);

	float PowerOutput_get() const;

	Vec2F m_Pos;

	SunPart m_Parts[SUN_PART_COUNT];

	PowerSink m_PowerSink;

	CD_TYPE m_SelectionId;
};
