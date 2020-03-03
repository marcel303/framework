#include "GeoBoneInfluence.h"

namespace Geo
{

BoneInfluence::BoneInfluence()
{

	type = bitNone;
	
	m_position = Vector(0.0f, 0.0f, 0.0f);
	m_rotation.MakeIdentity();
	
}

BoneInfluence::~BoneInfluence()
{

}

float BoneInfluence::CalculateInfluenceGlobal(const Vector& position) const
{

	Vector temp = m_transformGlobalInverse * position;
	
	return CalculateInfluence(temp);
	
}

float BoneInfluence::CalculateInfluenceLocal(const Vector& position) const
{

	Vector temp = m_transformInverse * position;
	
	return CalculateInfluence(temp);
	
}

float BoneInfluence::CalculateInfluence(const Vector& position) const
{

	return 0.0f;
	
}

bool BoneInfluence::Finalize()
{

	Matrix translation;
	translation.MakeTranslation(m_position);
	
	m_transform = translation * m_rotation;
	
	m_transformInverse = m_transform.Inverse();
	
	return true;
	
}

};
