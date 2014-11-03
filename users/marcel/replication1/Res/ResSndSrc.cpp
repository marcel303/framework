#include "ResSndSrc.h"

ResSndSrc::ResSndSrc()
{
	SetType(RES_SND_SRC);
}

void ResSndSrc::SetPosition(const Vec3& position)
{
	m_position = position;

	Invalidate();
}

void ResSndSrc::SetVelocity(const Vec3& velocity)
{
	m_velocity = velocity;

	Invalidate();
}
