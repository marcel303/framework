#ifndef ENTITYLINK_H
#define ENTITYLINK_H
#pragma once

#include "Entity.h"

template <class T>
class EntityLink
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

	ShParameter GetParameter();

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

	int32_t m_id;
	Entity* m_owner;
	ShParameter m_parameter;
};

//

template <class T>
EntityLink<T>::EntityLink(const std::string& name, Entity* owner)
{
	FASSERT(name != "");
	FASSERT(owner);

	m_owner = owner;

	m_parameter = ShParameter(new Parameter(PARAM_INT32, name, REP_ONCHANGE, COMPRESS_NONE, &m_id));
}

template <class T>
void EntityLink<T>::Initialize(int id)
{
	m_id = id;

	m_parameter->Invalidate();
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

	if (!scene->FindEntity(m_id).get())
		return false;

	return true;
}

template <class T>
Entity* EntityLink<T>::Get() const
{
	return DoGet();
}

template <class T>
ShParameter EntityLink<T>::GetParameter()
{
	return m_parameter;
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

	return m_owner->m_scene->FindEntity(m_id).get();
}

#endif
