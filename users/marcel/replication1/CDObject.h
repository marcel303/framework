#ifndef CDOBJECT_H
#define CDOBJECT_H
#pragma once

#include "CDContact.h"
#include "SharedPtr.h"
#include "Vec3.h"

namespace CD
{
	class Object
	{
	public:
		enum TYPE
		{
			TYPE_CUBE,
			TYPE_SPHERE,
		};

		Object(TYPE type);
		virtual ~Object();

		inline TYPE GetType() const;
		inline void* GetUP() const;
		inline const Vec3& GetPosition() const;
		inline void SetType(TYPE type);
		inline void SetUP(void* up);
		inline void SetPosition(const Vec3& position);

		virtual void OnPositionChange(const Vec3& position);

		virtual bool HitRay(const Vec3& position, const Vec3& direction, float maxDistance, float& out_distance);
		virtual bool IntersectPoint(const Vec3& position, Contact& out_contact);
		virtual bool IntersectCube(const Vec3& min, const Vec3& max, Contact& out_contact);

		virtual void Debug_Render();

	private:
		TYPE m_type;
		void* m_up;
		Vec3 m_position;
	};

	typedef SharedPtr<Object> ShObject;
}

#include "CDObject.inl"

#endif
