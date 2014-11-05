#ifndef CDSCENE_H
#define CDSCENE_H
#pragma once

#include <map>
#include "CDGroupFLT.h"
#include "CDObject.h"

namespace CD
{
	class Scene
	{
	public:
		Scene();
		~Scene();

		void AddObject(int group, Object* object);
		void RemoveObject(Object* object);
		void Clear();

		Object* CastRay(GroupFlt* filter, const Vec3& position, const Vec3& direction, float maxDistance, float* out_distance);
		int IntersectPoint(int excludeGroup, const Vec3& position, Contact* out_contacts, int maxContacts);
		int IntersectCube(int excludeGroup, const Vec3& min, const Vec3& max, Contact* out_contacts, int maxContacts);

	private:
	public: // FIXME.
		class ObjectItem
		{
		public:
			ObjectItem(Object* object, int group);
			~ObjectItem();

			Object* m_object;
			int m_group;
		};

		typedef std::map<CD::Object*, ObjectItem*> ObjectColl;
		typedef ObjectColl::iterator ObjectCollItr;

		ObjectColl m_objects;
	};
}

#endif
