#pragma once

#include "Debugging.h"
#include <typeindex> // todo : remove and replace with opaque type wrapping type index or type hash

// todo : movement AngleAxis object elsewhere

#include "Vec3.h"

struct AngleAxis
{
	float angle = 0.f;
	Vec3 axis = Vec3(0.f, 1.f, 0.f);
};

//

struct ComponentBase;
struct ComponentMgrBase;
struct ComponentSet;

//

struct ComponentBase
{
	ComponentSet * componentSet = nullptr;
	ComponentBase * next_in_set = nullptr; // next component in the component set
	
	virtual void tick(const float dt) { }
	virtual bool init() { return true; }
	
	virtual std::type_index typeIndex() = 0;
};

template <typename T>
struct Component : ComponentBase
{
	T * next = nullptr;
	T * prev = nullptr;
	
	virtual std::type_index typeIndex() override
	{
		return std::type_index(typeid(T));
	}
};

struct ComponentMgrBase
{
	virtual ComponentBase * createComponent() = 0;
	virtual void addComponent(ComponentBase * component) = 0;
	virtual void removeComponent(ComponentBase * component) = 0;
	
	virtual void tick(const float dt) = 0;
	
	virtual std::type_index typeIndex() = 0;
};

template <typename T>
struct ComponentMgr : ComponentMgrBase
{
	T * head = nullptr;
	T * tail = nullptr;
	
	virtual T * createComponent() override
	{
		T * component = new T();
		
		addComponent(component);
		
		return component;
	}
	
	virtual void addComponent(ComponentBase * in_component) override
	{
		T * component = castToComponentType(in_component);
		
		Assert(component->prev == nullptr);
		Assert(component->next == nullptr);
		
		if (head == nullptr)
		{
			head = component;
			tail = component;
		}
		else
		{
			tail->next = component;
			component->prev = tail;
			
			tail = component;
		}
	}
	
	virtual void removeComponent(ComponentBase * in_component) override
	{
		T * component = castToComponentType(in_component);
		
		if (component->prev != nullptr)
			component->prev->next = component->next;
		if (component->next != nullptr)
			component->next->prev = component->prev;
		
		if (component == head)
			head = component->next;
		if (component == tail)
			tail = component->prev;
		
		component->prev = nullptr;
		component->next = nullptr;
	}
	
	virtual void tick(const float dt) override
	{
		for (T * i = head; i != nullptr; i = i->next)
		{
			i->tick(dt);
		}
	}
	
	inline T * castToComponentType(ComponentBase * component)
	{
		return static_cast<T*>(component);
	}
	
	virtual std::type_index typeIndex() override
	{
		return std::type_index(typeid(T));
	}
};

struct ComponentSet
{
	ComponentBase * head = nullptr;
	
	void add(ComponentBase * component)
	{
		component->next_in_set = head;
		head = component;
	}
	
	template <typename T>
	T * findComponent()
	{
		for (auto * component = head; component != nullptr; component = component->next_in_set)
		{
			if (component->typeIndex() == std::type_index(typeid(T)))
				return static_cast<T*>(component);
		}
		
		return nullptr;
	}
	
	template <typename T>
	const T * findComponent() const
	{
		for (auto * component = head; component != nullptr; component = component->next_in_set)
		{
			if (component->typeIndex() == std::type_index(typeid(T)))
				return static_cast<T*>(component);
		}
		
		return nullptr;
	}
	
};
