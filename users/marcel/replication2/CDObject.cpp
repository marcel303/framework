#include "CDObject.h"

namespace CD
{
	Object::Object(TYPE type)
	{
		m_type = type;
		m_up = 0;
		m_position = Vec3(0.0f, 0.0f, 0.0f);
	}

	Object::~Object()
	{
	}

	void Object::OnPositionChange(const Vec3& position)
	{
	}

	bool Object::HitRay(const Vec3& position, const Vec3& direction, float maxDistance, float& out_distance)
	{
		return false;
	}

	bool Object::IntersectPoint(const Vec3& position, Contact& out_contact)
	{
		return false;
	}

	bool Object::IntersectCube(const Vec3& min, const Vec3& max, Contact& out_contact)
	{
		return false;
	}

	void Object::Debug_Render()
	{
	}
}
