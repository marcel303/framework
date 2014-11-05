#ifndef ENTITYLINK_H
#define ENTITYLINK_H
#pragma once

#include "Entity.h"

template <class T>
class EntityLink : public NetSerializable
{
public:
	EntityLink(const std::string& name, Entity* owner);

	void Initialize(int id);
	void Initialize(const ShEntity & entity);

	bool Valid() const;
	Entity* Get() const;
	inline T* GetT() const
	{
		return (T*)Get();
	}

	void operator=(int id);
	void operator=(const ShEntity & entity);
	inline Entity* operator->()
	{
		return DoGet();
	}
	inline const Entity* operator->() const
	{
		return DoGet();
	}

private:
	Entity* DoGet() const;

	virtual void SerializeStruct()
	{
		m_id.Serialize(this);
	}

	NetValue<int32_t> m_id;
	Entity* m_owner;
};

//

template <class T>
EntityLink<T>::EntityLink(const std::string& name, Entity* owner)
	: NetSerializable(owner)
	, m_id(this)
{
	FASSERT(name != "");
	FASSERT(owner);

	m_owner = owner;
}

template <class T>
void EntityLink<T>::Initialize(int id)
{
	m_id = id;
}

template <class T>
void EntityLink<T>::Initialize(const ShEntity & entity)
{
	if (entity.get() != 0)
		Initialize(entity->m_id);
	else
		Initialize(0);
}

template <class T>
bool EntityLink<T>::Valid() const
{
	if (!m_owner)
		return false;

	Scene* scene = m_owner->m_scene;

	if (!scene)
		return false;

	if (!scene->FindEntity(m_id.get()).get())
		return false;

	return true;
}

template <class T>
Entity* EntityLink<T>::Get() const
{
	return DoGet();
}

template <class T>
void EntityLink<T>::operator=(int id)
{
	Initialize(id);
}

template <class T>
void EntityLink<T>::operator=(const ShEntity & entity)
{
	Initialize(entity);
}

template <class T>
Entity* EntityLink<T>::DoGet() const
{
	if (m_owner->m_scene == 0)
		return 0;

	return m_owner->m_scene->FindEntity(m_id.get()).get();
}

#endif
