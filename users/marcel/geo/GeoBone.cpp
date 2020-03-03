#include "GeoBone.h"

namespace Geo
{

	Bone::Bone()
	{

		parent = nullptr;
		
		// Initialize bind and current position / rotation.
		
		bindRotationLocal.MakeIdentity();
		bindPositionLocal.SetZero();

		currentRotationLocal.MakeIdentity();
		currentPositionLocal.SetZero();

		// Finalize (calculate bind transforms). This will also update current transforms.
		
		Finalize();
		
	}

	Bone::~Bone()
	{

		// Remove influences.
		
		for (std::list<BoneInfluence*>::iterator i = influences.begin(); i != influences.end(); ++i)
		{
		
			delete (*i);
			
		}
		
		// Remove child bones.
		
		for (std::list<Bone*>::iterator i = children.begin(); i != children.end(); ++i)
		{
		
			delete (*i);
			
		}
		
	}
	
	float Bone::CalculateInfluence(Vec3Arg position) const
	{

		// Calculate cumulative influence.
		
		float influence = 0.0f;
		
		for (std::list<BoneInfluence*>::const_iterator i = influences.begin(); i != influences.end(); ++i)
		{

			influence += (*i)->CalculateInfluenceGlobal(position);
			
		}

		return influence;
		
	}

	bool Bone::Finalize()
	{

		// Calculate bind transform and it's inverse.
		
		Mat4x4 bindTranslation;
		bindTranslation.MakeTranslation(bindPositionLocal);
		
		bindTransformGlobal = bindTranslation * bindRotationLocal;

		if (parent != nullptr)
		{
			bindTransformGlobal = parent->bindTransformGlobal * bindTransformGlobal;
		}

		bindTransformGlobalInverse = bindTransformGlobal.CalcInv();
		
		// Finalize children.
		
		for (std::list<Bone*>::iterator i = children.begin(); i != children.end(); ++i)
		{
		
			(*i)->parent = this;
			
			if (!(*i)->Finalize())
			{
				return false;
			}
		}
		
		// Finalize influences.
		
		for (std::list<BoneInfluence*>::iterator i = influences.begin(); i != influences.end(); ++i)
		{
			if (!(*i)->Finalize())
			{
				return false;
			}
			
			(*i)->transformGlobal = bindTransformGlobal * (*i)->transform;
			(*i)->transformGlobalInverse = (*i)->transformGlobal.CalcInv();
			
		}
		
		// Updat current transforms.
		
		Update();
		
		return true;
		
	}

	void Bone::SetCurrentToBind()
	{

		// Copy bind position & transform to current position and transform.
		
		currentPositionLocal = bindPositionLocal;
		currentRotationLocal = bindRotationLocal;
		
		for (std::list<Bone*>::iterator i = children.begin(); i != children.end(); ++i)
		{
			(*i)->SetCurrentToBind();
		}
		
	}

	bool Bone::Update()
	{

		// Calculate current transform and it's inverse.
		
		Mat4x4 currentTranslation;
		currentTranslation.MakeTranslation(currentPositionLocal);
		
		currentTransformGlobal = currentTranslation * currentRotationLocal;

		if (parent != nullptr)
		{
			currentTransformGlobal = parent->currentTransformGlobal * currentTransformGlobal;
		}
		
		transform = currentTransformGlobal * bindTransformGlobalInverse;
		
		// Update children.
		
		for (std::list<Bone*>::iterator i = children.begin(); i != children.end(); ++i)
		{
			if (!(*i)->Update())
			{
				return false;
			}
		}
		
		return true;
		
	}
	
}
