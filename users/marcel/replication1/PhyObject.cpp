#include "Debug.h"
#include "PhyObject.h"

namespace Phy
{
	Object::Object()
	{
		Initialize(0, Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 0.0f), false, false, (void*)0xbaadf33d);
	}

	Object::~Object()
	{
		m_geometry.clear();
	}

	void Object::NextFrame()
	{
		m_force = Vec3(0.0f, 0.0f, 0.0f);

		// FIXME: Move geometry to new position.

		//printf("NextFrame: S = (%+03.3f, %+03.3f, %+03.3f). V = (%+03.3f, %+03.3f, %+03.3f).\n", m_position[0], m_position[1], m_position[2], m_velocity[0], m_velocity[1], m_velocity[2]);
	}

	void Object::Initialize(int group, const Vec3& position, const Vec3& velocity, bool dynamic, bool gravity, void* up)
	{
		FASSERT(up);

		m_group = group;
		m_velocity = velocity;
		m_velocityDamp = Vec3(1.0f, 1.0f, 1.0f);
		m_dynamic = dynamic;
		m_gravity = gravity;
		m_force = Vec3(0.0f, 0.0f, 0.0f);
		m_up = up;

		SetPosition(position);
	}

	void Object::AddGeometry(CD::ShObject object)
	{
		FASSERT(object.get());
		FASSERT(m_up);

		object->SetUP(m_up);
		object->SetPosition(m_position);

		m_geometry.push_back(object);
	}

	void Object::SetPosition(Vec3 position)
	{
		m_position = position;

		for (size_t i = 0; i < m_geometry.size(); ++i)
			m_geometry[i]->SetPosition(m_position);
	}

	void Object::Debug_Render()
	{
		for (size_t i = 0; i < m_geometry.size(); ++i)
			m_geometry[i]->Debug_Render();
	}
}
