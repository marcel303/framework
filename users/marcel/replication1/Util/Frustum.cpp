#include "Frustum.h"

void Frustum::Setup(const Mat4x4& matV, float nearDistance, float farDistance)
{
	Mat4x4 invMatV = matV.CalcInv();

	Vec3 p[4] =
	{
		invMatV * Vec3(-1.0f, -1.0f, 1.0f),
		invMatV * Vec3(+1.0f, -1.0f, 1.0f),
		invMatV * Vec3(+1.0f, +1.0f, 1.0f),
		invMatV * Vec3(-1.0f, +1.0f, 1.0f)
	};

	Vec3 origin = invMatV.Mul4(Vec3(0.0f, 0.0f, 0.0f));
	Vec3 normal1 = invMatV.Mul(Vec3(0.0f, 0.0f, 1.0f));
	Vec3 normal2 = -normal1;

	m_planes[0].SetupCCW(origin, p[0], p[1]);
	m_planes[1].SetupCCW(origin, p[1], p[2]);
	m_planes[2].SetupCCW(origin, p[2], p[3]);
	m_planes[3].SetupCCW(origin, p[3], p[0]);

	m_planes[4].Setup(normal1, normal1 * (origin + normal1 * nearDistance));
	m_planes[5].Setup(normal2, normal2 * (origin + normal1 * farDistance));
}

Mx::CLASS Frustum::Classify(const AABB& aabb, Mat4x4 transform)
{
	int inCnt = 0;
	int outCnt = 0;

	Vec3 p[8];

	aabb.Transform(transform, p);

	for (int j = 0; j < 8; ++j)
	{
		bool out = false;

		for (int i = 0; i < 6; ++i)
		{
			if (m_planes[i] * p[j] < 0.0f)
				out = true;
		}

		if (out)
			outCnt++;
		else
			inCnt++;
	}

	if (inCnt == 0)
		return Mx::CLASS_OUT;
	if (outCnt == 0)
		return Mx::CLASS_IN;

	return Mx::CLASS_SPAN;
}