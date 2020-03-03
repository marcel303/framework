#include "GeoBoneInfluence.h"

namespace Geo
{

	BoneInfluence::BoneInfluence()
	{

		type = bitNone;
		
		position.SetZero();
		rotation.MakeIdentity();
		
	}

	BoneInfluence::~BoneInfluence()
	{

	}

	float BoneInfluence::CalculateInfluenceGlobal(Vec3Arg in_position) const
	{

		const Vec3 temp = transformGlobalInverse * in_position;
		
		return CalculateInfluence(temp);
		
	}

	float BoneInfluence::CalculateInfluenceLocal(Vec3Arg in_position) const
	{

		const Vec3 temp = transformInverse * in_position;
		
		return CalculateInfluence(temp);
		
	}

	float BoneInfluence::CalculateInfluence(Vec3Arg position) const
	{

		return 0.0f;
		
	}

	bool BoneInfluence::Finalize()
	{

		Mat4x4 translation;
		translation.MakeTranslation(position);
		
		transform = translation * rotation;
		
		transformInverse = transform.CalcInv();
		
		return true;
		
	}

};
