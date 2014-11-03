#ifndef RESSNDSRC_H
#define RESSNDSRC_H
#pragma once

#include "Res.h"
#include "Vec3.h"

class ResSndSrc : public Res
{
public:
	ResSndSrc();

	void SetPosition(const Vec3& position);
	void SetVelocity(const Vec3& velocity);

	Vec3 m_position;
	Vec3 m_velocity;
};

#endif
