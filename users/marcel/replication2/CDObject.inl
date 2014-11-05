#ifndef CDOBJECT_INL
#define CDOBJECT_INL
#pragma once

namespace CD
{
	inline Object::TYPE Object::GetType() const
	{
		return m_type;
	}

	inline void* Object::GetUP() const
	{
		return m_up;
	}

	inline const Vec3& Object::GetPosition() const
	{
		return m_position;
	}

	inline void Object::SetType(TYPE type)
	{
		m_type = type;
	}

	inline void Object::SetUP(void* up)
	{
		m_up = up;
	}

	inline void Object::SetPosition(const Vec3& position)
	{
		m_position = position;

		OnPositionChange(m_position);
	}
}

#endif
