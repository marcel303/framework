#include "GeoBoneInfluence.h"

namespace Geo
{

	BoneInfluence::BoneInfluence()
	{

		type = bitNone;
		
		position = Vector(0.0f, 0.0f, 0.0f);
		rotation.MakeIdentity();
		
	}

	BoneInfluence::~BoneInfluence()
	{

	}

	float BoneInfluence::CalculateInfluenceGlobal(const Vector& in_position) const
	{

		const Vector temp = transformGlobalInverse * in_position;
		
		return CalculateInfluence(temp);
		
	}

	float BoneInfluence::CalculateInfluenceLocal(const Vector& in_position) const
	{

		const Vector temp = transformInverse * in_position;
		
		return CalculateInfluence(temp);
		
	}

	float BoneInfluence::CalculateInfluence(const Vector& position) const
	{

		return 0.0f;
		
	}

	bool BoneInfluence::Finalize()
	{

		Matrix translation;
		translation.MakeTranslation(position);
		
		transform = translation * rotation;
		
		transformInverse = transform.Inverse();
		
		return true;
		
	}

};
