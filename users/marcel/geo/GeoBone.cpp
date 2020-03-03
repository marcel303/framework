#include "GeoBone.h"

namespace Geo
{

	Bone::Bone()
	{

		m_parent = 0;
		
		// Initialize bind and current position / rotation.
		
		m_bindRotationLocal.MakeIdentity();
		m_bindPositionLocal = Vector(0.0f, 0.0f, 0.0f);

		m_currentRotationLocal.MakeIdentity();
		m_currentPositionLocal = Vector(0.0f, 0.0f, 0.0f);

		// Finalize (calculate bind transforms). This will also update current transforms.
		
		Finalize();
		
	}

	Bone::~Bone()
	{

		// Remove influences.
		
		for (std::list<BoneInfluence*>::iterator i = cInfluence.begin(); i != cInfluence.end(); ++i)
		{
		
			delete (*i);
			
		}
		
		// Remove child bones.
		
		for (std::list<Bone*>::iterator i = cChild.begin(); i != cChild.end(); ++i)
		{
		
			delete (*i);
			
		}
		
	}
	
	float Bone::CalculateInfluence(const Vector& position) const
	{

		// Calculate cumulative influence.
		
		float influence = 0.0f;
		
		for (std::list<BoneInfluence*>::const_iterator i = cInfluence.begin(); i != cInfluence.end(); ++i)
		{

			influence += (*i)->CalculateInfluenceGlobal(position);
			
		}

		return influence;
		
	}

	bool Bone::Finalize()
	{

		// Calculate bind transform and it's inverse.
		
		Matrix bindTranslation;
		bindTranslation.MakeTranslation(m_bindPositionLocal);
		
		m_bindTransformGlobal = bindTranslation * m_bindRotationLocal;

		if (m_parent)
		{
			m_bindTransformGlobal = m_parent->m_bindTransformGlobal * m_bindTransformGlobal;
		}

		m_bindTransformGlobalInverse = m_bindTransformGlobal.Inverse();
		
		// Finalize children.
		
		for (std::list<Bone*>::iterator i = cChild.begin(); i != cChild.end(); ++i)
		{
		
			(*i)->m_parent = this;
			
			if (!(*i)->Finalize())
			{
				return false;
			}
		}
		
		// Finalize influences.
		
		for (std::list<BoneInfluence*>::iterator i = cInfluence.begin(); i != cInfluence.end(); ++i)
		{
			if (!(*i)->Finalize())
			{
				return false;
			}
			
			(*i)->m_transformGlobal = m_bindTransformGlobal * (*i)->m_transform;
			(*i)->m_transformGlobalInverse = (*i)->m_transformGlobal.Inverse();
			
		}
		
		// Updat current transforms.
		
		Update();
		
		return true;
		
	}

	void Bone::SetCurrentToBind()
	{

		// Copy bind position & transform to current position and transform.
		
		m_currentPositionLocal = m_bindPositionLocal;
		m_currentRotationLocal = m_bindRotationLocal;
		
		for (std::list<Bone*>::iterator i = cChild.begin(); i != cChild.end(); ++i)
		{
			(*i)->SetCurrentToBind();
		}
		
	}

	bool Bone::Update()
	{

		// Calculate current transform and it's inverse.
		
		Matrix currentTranslation;
		currentTranslation.MakeTranslation(m_currentPositionLocal);
		
		m_currentTransformGlobal = currentTranslation * m_currentRotationLocal;

		if (m_parent)
		{
			m_currentTransformGlobal = m_parent->m_currentTransformGlobal * m_currentTransformGlobal;
		}
		
		m_transform = m_currentTransformGlobal * m_bindTransformGlobalInverse;
		
		// Update children.
		
		for (std::list<Bone*>::iterator i = cChild.begin(); i != cChild.end(); ++i)
		{
			if (!(*i)->Update())
			{
				return false;
			}
		}
		
		return true;
		
	}
	
}
