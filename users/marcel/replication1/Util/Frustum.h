#ifndef FRUSTUM_H
#define FRUSTUM_H
#pragma once

#include "AABB.h"
#include "MxClass.h"
#include "MxPlane.h"

class Frustum
{
public:
	inline Frustum()
	{
	}

	void Setup(const Mat4x4& matV, float nearDistance, float farDistance);
	Mx::CLASS Classify(const AABB& aabb, Mat4x4 transform);

	Mx::Plane m_planes[6];
	Vec3 m_vertices[8];
};

#endif
