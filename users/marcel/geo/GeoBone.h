#pragma once

#include "GeoBoneInfluence.h"
#include "Mat4x4.h"
#include "Vec3.h"
#include <list>

namespace Geo
{

	/// Geometry: Skeletal bone type.
	/**
	 * A skeletal bone is used when creating a skeletal structure and during animation.
	 */
	class Bone
	{

	public:
		
		Bone();
		~Bone();
		
	public:
		
		std::list<BoneInfluence*> influences; ///< List of influences. Used to calculate influence this bone has on vertices.
		
	public:
		
		float CalculateInfluence(Vec3Arg position) const; ///< Calculate weight at given position in global coordinate system.
		
	public:
		
		Vec3 bindPositionLocal; ///< Position at bind time.
		Mat4x4 bindRotationLocal; ///< Rotation at bind time.
		Mat4x4 bindTransformGlobal; ///< Combined matrix that transforms vertices in local bone coordinate system to position in global coordinate system.
		Mat4x4 bindTransformGlobalInverse; ///< Combined matrix that transforms vertices at bind time to local bone coordinate system.
		
		Vec3 currentPositionLocal; ///< Current position in local coordinate system.
		Mat4x4 currentRotationLocal; ///< Current rotation in local coordinate system.
		Mat4x4 currentTransformGlobal; ///< Combined transform from root uptil this bone.
		
		Mat4x4 transform; ///< Final transform matrix that modifies vertices under bone rotation and position.
		
	public:
		
		Bone* parent; ///< Parent bone, or null if root.
		
		std::list<Bone*> children; ///< List of child bones.
		
	public:
		
		bool Finalize(); ///< Finalize bone hierarchy at bind time. Setup bind matrices. Calculate inverse transforms.
		
		void SetCurrentToBind(); ///< Set current position & rotation to match position & rotation at bind time.
		
		bool Update(); ///< Update bone hierarchy after modifying current state. Update final transform given new current position & rotation.

	};

}
