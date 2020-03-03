#pragma once

#include "MathMatrix.h"
#include "MathVector.h"

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
		
		Vector position; ///< Local position.
		Matrix rotation; ///< Local rotation.
		
	public:
		
		Matrix transform; ///< Local transform. Calculated at finalization phase.
		Matrix transformInverse; ///< Inverse of local transform. Calculated at finalization phase.
		
	public:
		
		Matrix transformGlobal; ///< Global transform. Calculated at bone finalization phase.
		Matrix transformGlobalInverse; /// Inverse of global transform. Calculated at bone finalization phase.

	public:
		
		float CalculateInfluenceGlobal(const Vector& position) const; ///< Calculate influence at given position in global coordinates.
		float CalculateInfluenceLocal(const Vector& position) const; ///< Calculate influence at given position in global bone coordinates.
		
		virtual float CalculateInfluence(const Vector& position) const; ///< Calculate influence at given position in global influence coordinates.
		
		virtual bool Finalize(); ///< Finalize. Must be called after either modifying position or rotation.
		
	};

}
