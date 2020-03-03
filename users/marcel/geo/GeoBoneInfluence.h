#pragma once

#include "Mat4x4.h"
#include "Vec3.h"

namespace Geo
{

	/// Bone influence class.
	enum BoneInfluenceType
	{
		bitNone,           ///< Unknown class.
		bitCylinderCapped, ///< Capped cylinder class.
		bitSphere          ///< Sphere class.
	};

	/// Geometry: Bone influence type.
	/**
	 * One or more bone influences are associated with a bone to determine a bone's weight when skinning a mesh.
	 */
	class BoneInfluence
	{

	public:
		
		BoneInfluence();
		virtual ~BoneInfluence();

	public:
		
		BoneInfluenceType type; ///< Type of bone influence. May be used when rendering a bone hierarchy + influences.
		
	public:
		
		Vec3 position; ///< Local position.
		Mat4x4 rotation; ///< Local rotation.
		
	public:
		
		Mat4x4 transform; ///< Local transform. Calculated at finalization phase.
		Mat4x4 transformInverse; ///< Inverse of local transform. Calculated at finalization phase.
		
	public:
		
		Mat4x4 transformGlobal; ///< Global transform. Calculated at bone finalization phase.
		Mat4x4 transformGlobalInverse; /// Inverse of global transform. Calculated at bone finalization phase.

	public:
		
		float CalculateInfluenceGlobal(Vec3Arg position) const; ///< Calculate influence at given position in global coordinates.
		float CalculateInfluenceLocal(Vec3Arg position) const; ///< Calculate influence at given position in global bone coordinates.
		
		virtual float CalculateInfluence(Vec3Arg position) const; ///< Calculate influence at given position in global influence coordinates.
		
		virtual bool Finalize(); ///< Finalize. Must be called after either modifying position or rotation.
		
	};

}
