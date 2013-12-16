#include "GoSun.h"
#include "Map.h"
#include "Renderer.h"

Sun::Sun(Map* map) : IObject(ObjectType_Sun, map)
{
	m_PowerSink.Capacity_set(400.0f);

	m_SelectionId = map->m_SelectionMap->Allocate();
	map->m_SelectionMap->Set(m_SelectionId, this);
}

void Sun::Setup(Vec2F pos)
{
	m_Pos = pos;

	MaskMap* partMasks[4] =
	{
		new MaskMap(),
		new MaskMap(),
		new MaskMap(),
		new MaskMap()
	};

	partMasks[0]->Load("sun_part_0_mask.bmp");
	partMasks[1]->Load("sun_part_1_mask.bmp");
	partMasks[2]->Load("sun_part_2_mask.bmp");
	partMasks[3]->Load("sun_part_3_mask.bmp");

	m_Parts[0].m_Pos = pos + Vec2F(-40, -40);
	m_Parts[1].m_Pos = pos + Vec2F(+40, -40);
	m_Parts[2].m_Pos = pos + Vec2F(+40, +40);
	m_Parts[3].m_Pos = pos + Vec2F(-40, +40);

	for (int i = 0; i < 4; ++i)
		m_Parts[i].m_Pos -= Vec2F(partMasks[i]->m_Sx / 2.0f, partMasks[i]->m_Sy / 2.0f);

	m_Parts[0].m_Mask = partMasks[0];
	m_Parts[1].m_Mask = partMasks[1];
	m_Parts[2].m_Mask = partMasks[2];
	m_Parts[3].m_Mask = partMasks[3];
}

BOOL Sun::GetPartAngles(int index, float& o_Angle1, float& o_Angle2)
{
	float step = 2.0f * M_PI / SUN_PART_COUNT;

	o_Angle1 = (index + 0) * step;
	o_Angle2 = (index + 1) * step;

	return !m_Parts[index].m_IsDead;
}

SunPart* Sun::GetPart(Vec2F pos)
{
	Vec2F d = pos - m_Pos;

	float angle = atan2(d[0], d[1]);

	for (int i = 0; i < SUN_PART_COUNT; ++i)
	{
		float a1;
		float a2;

		GetPartAngles(i, a1, a2);
	
		if (angle >= a1 && angle <= a2)
			return &m_Parts[i];
	}

	return 0;
}

void Sun::Hit(Vec2F pos, int hitPoints)
{
	SunPart* part = GetPart(pos);

	if (part->m_IsDead)
		return;

	part->Hit(hitPoints);
}

void Sun::Update(Map* map)
{
	m_PowerSink.Store(PowerOutput_get() * dt);

	m_PowerSink.DistributionCommit();

	m_PowerSink.Update();
}

void Sun::Render(BITMAP* buffer)
{
	g_Renderer.Circle(
		buffer, 
		m_Pos,
		10,
		makecol(255, 255, 0));

	g_Renderer.Circle(
		buffer, 
		m_Pos,
		25,
		makecol(127, 127, 0));
}

void Sun::RenderSB(SelectionBuffer* sb)
{
	g_Renderer.Circle(
		sb,
		m_Pos,
		25,
		m_SelectionId);

	for (int i = 0; i < 4; ++i)
	{
		g_Renderer.MaskMap(
			sb,
			m_Parts[i].m_Mask,
			m_Parts[i].m_Pos,
			m_SelectionId); // fixme
			//m_Parts[i].m_SelectionId);
	}
}

float Sun::PowerOutput_get() const
{
	float maxOutput = 100.0f;
	float step = maxOutput / SUN_PART_COUNT;

	float result = 0.0f;

	for (int i = 0; i < SUN_PART_COUNT; ++i)
	{
		if (m_Parts[i].m_IsDead)
			continue;

		result += step;
	}

	return result;
}
