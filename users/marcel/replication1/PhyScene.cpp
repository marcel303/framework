#include "Debug.h"
#include "PhyScene.h"

static Vec3 Pow(Vec3Arg v, float t) // todo : make util function
{
	return Vec3(pow(v[0], t), pow(v[1], t), pow(v[2], t));
}

namespace Phy
{
	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
		Clear();
	}

	void Scene::AddObject(Object* object)
	{
		FASSERT(object);

		m_objects[object] = 1;

		// Add CD objects.
		for (size_t i = 0; i < object->GetGeometry().size(); ++i)
			m_cdScene.AddObject(object->m_group, object->GetGeometry()[i].get());
	}

	void Scene::RemoveObject(Object* object)
	{
		FASSERT(object);

		// Remove CD objects.
		for (size_t i = 0; i < object->GetGeometry().size(); ++i)
			m_cdScene.RemoveObject(object->GetGeometry()[i].get());

		ObjectCollItr j = m_objects.find(object);
		m_objects.erase(j);
	}

	void Scene::Clear()
	{
		while (m_objects.size() > 0)
			RemoveObject(m_objects.begin()->first);
	}

	void Scene::Gravity(Vec3 force)
	{
		for (ObjectCollItr i = m_objects.begin(); i != m_objects.end(); ++i)
		{
			Object* object = i->first;

			if (object->m_gravity)
				object->m_force += force;
		}
	}

	void Scene::Update(float dt)
	{
		for (ObjectCollItr i = m_objects.begin(); i != m_objects.end(); ++i)
		{
			Object* object = i->first;

			if (object->m_dynamic)
			{
				// Find contacts.
				const int MAX_CONTACTS = 16;

				CD::Contact contacts[MAX_CONTACTS];
				int contactCount = 0;

				// Could use a little broad phase..
				for (size_t j = 0; j < object->GetGeometry().size(); ++j)
				{
					if (object->GetGeometry()[j]->GetType() == CD::Object::TYPE_CUBE)
					{
						CD::Cube* cube = (CD::Cube*)object->GetGeometry()[j].get();
						contactCount += m_cdScene.IntersectCube(object->m_group, cube->m_absMin, cube->m_absMax, &contacts[contactCount], MAX_CONTACTS - contactCount);
					}
				}

				// Clipping.
				for (int j = 0; j < contactCount; ++j)
				{
					// Clip position.
					object->m_position += contacts[j].m_plane.m_normal * contacts[j].m_plane.m_distance;
					// Clip speed.
					float v = contacts[j].m_plane.m_normal * object->m_velocity;
					object->m_velocity -= contacts[j].m_plane.m_normal * v;
				}

				Vec3 damp = Pow(object->m_velocityDamp, dt);

				object->m_velocity = Vec3(
					object->m_velocity[0] * damp[0],
					object->m_velocity[1] * damp[1],
					object->m_velocity[2] * damp[2]);
			}
		}

		for (ObjectCollItr i = m_objects.begin(); i != m_objects.end(); ++i)
		{
			Object* object = i->first;

			if (object->m_dynamic)
			{
				// Apply forces.
				object->m_velocity += object->m_force * dt;

				Vec3 position = object->m_position + object->m_velocity * dt;

				object->SetPosition(position);

				// Reset forces.
				object->NextFrame();
			}
		}
	}

	CD::Scene* Scene::GetCDScene()
	{
		return &m_cdScene;
	}

	void Scene::Debug_Render()
	{
		for (ObjectCollItr i = m_objects.begin(); i != m_objects.end(); ++i)
		{
			Object* object = i->first;

			object->Debug_Render();
		}
	}
}
