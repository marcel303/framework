#pragma once

#include "GeoBoneInfluence.h"
#include "MathMatrix.h"
#include "MathVector.h"
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
		
		std::list<BoneInfluence*> cInfluence; ///< List of influences. Used to calculate influence this bone has on vertices.
		
	public:
		
		float CalculateInfluence(const Vector& position) const; ///< Calculate weight at given position in global coordinate system.
		
	public:
		
		Vector m_bindPositionLocal; ///< Position at bind time.
		Matrix m_bindRotationLocal; ///< Rotation at bind time.
		Matrix m_bindTransformGlobal; ///< Combined matrix that transforms vertices in local bone coordinate system to position in global coordinate system.
		Matrix m_bindTransformGlobalInverse; ///< Combined matrix that transforms vertices at bind time to local bone coordinate system.
		
		Vector m_currentPositionLocal; ///< Current position in local coordinate system.
		Matrix m_currentRotationLocal; ///< Current rotation in local coordinate system.
		Matrix m_currentTransformGlobal; ///< Combined transform from root uptil this bone.
		
		Matrix m_transform; ///< Final transform matrix that modifies vertices under bone rotation and position.
		
	public:
		
		Bone* m_parent; ///< Parent bone, or 0 if root.
		
		std::list<Bone*> cChild; ///< List of child bones.
		
	public:
		
		bool Finalize(); ///< Finalize bone hierarchy at bind time. Setup bind matrices. Calculate inverse transforms.
		void SetCurrentToBind(); ///< Set current position & rotation to match position & rotation at bind time.
		bool Update(); ///< Update bone hierarchy after modifying current state. Update final transform given new current position & rotation.

	};

}
