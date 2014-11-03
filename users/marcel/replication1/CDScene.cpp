#include "Debug.h"
#include "CDScene.h"

namespace CD
{
	Scene::Scene()
	{
	}

	Scene::~Scene()
	{
		Clear();
	}

	void Scene::AddObject(int group, Object* object)
	{
		FASSERT(object);
		FASSERT(object->GetUP());

		m_objects[object] = new ObjectItem(object, group);
	}

	void Scene::RemoveObject(Object* object)
	{
		FASSERT(object);

		ObjectCollItr i = m_objects.find(object);
		delete i->second;
		m_objects.erase(i);
	}

	void Scene::Clear()
	{
		while (m_objects.size() > 0)
			RemoveObject(m_objects.begin()->first);
	}

	Object* Scene::CastRay(GroupFlt* filter, const Vec3& position, const Vec3& direction, float maxDistance, float* out_distance)
	{
		Object* object = 0;
		float distance = 0.0f;

		for (ObjectCollItr i = m_objects.begin(); i != m_objects.end(); ++i)
		{
			if (filter)
				if (!filter->Accept(i->second->m_group, i->second->m_object))
					continue;

			float distance2 = 0.0f;

			if (i->second->m_object->HitRay(position, direction, maxDistance, distance2))
			{
				if (object == 0 || distance2 < distance)
				{
					object = i->second->m_object;
					distance = distance2;
				}
			}
		}

		if (object)
		{
			//printf("CastRay: Hit at distance %f.\n", distance);

			if (out_distance)
				*out_distance = distance;
		}

		return object;
	}

	int Scene::IntersectPoint(int excludeGroup, const Vec3& position, Contact* out_contacts, int maxContacts)
	{
		int contactCount = 0;

		for (ObjectCollItr i = m_objects.begin(); i != m_objects.end(); ++i)
		{
			if (i->second->m_group != excludeGroup)
			{
				Contact contact;

				if (i->second->m_object->IntersectPoint(position, out_contacts[contactCount]))
				{
					++contactCount;

					if (contactCount == maxContacts)
						break;
				}
			}
		}

		return contactCount;
	}

	int Scene::IntersectCube(int excludeGroup, const Vec3& min, const Vec3& max, Contact* out_contacts, int maxContacts)
	{
		int contactCount = 0;

		for (ObjectCollItr i = m_objects.begin(); i != m_objects.end(); ++i)
		{
			if (i->second->m_group != excludeGroup)
			{
				Contact contact;

				if (i->second->m_object->IntersectCube(min, max, out_contacts[contactCount]))
				{
					++contactCount;

					if (contactCount == maxContacts)
						break;
				}
			}
		}

		return contactCount;
	}

	// Scene::ObjectItem
	Scene::ObjectItem::ObjectItem(Object* object, int group)
	{
		m_object = object;
		m_group = group;
	}

	Scene::ObjectItem::~ObjectItem()
	{
		//delete m_object;
	}
}
