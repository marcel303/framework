#pragma once

#include "Debugging.h"
#include <typeindex>

#define ENABLE_COMPONENT_JSON 1

//

struct ComponentBase;
struct ComponentMgrBase;
struct ComponentSet;

//

struct ComponentBase
{
	ComponentSet * componentSet = nullptr;
	ComponentBase * next_in_set = nullptr; // next component in the component set
	
	virtual ~ComponentBase();
	
	virtual void tick(const float dt) { }
	virtual bool init() { return true; }
	
	virtual std::type_index typeIndex() = 0;
	
	virtual void propertyChanged(void * address) { };
};

template <typename T>
struct Component : ComponentBase
{
	T * next = nullptr;
	T * prev = nullptr;
	
	virtual std::type_index typeIndex() override final
	{
		return std::type_index(typeid(T));
	}
};

struct ComponentMgrBase
{
	virtual ~ComponentMgrBase() { }
	
	virtual bool init() { return true; }
	virtual void shut() { }
	
	virtual ComponentBase * createComponent(const char * id) = 0;
	virtual void addComponent(ComponentBase * component) = 0;
	virtual void destroyComponentImpl(ComponentBase * component) = 0;
	
	template <typename T>
	void destroyComponent(T *& component)
	{
		destroyComponentImpl(component);
		component = nullptr;
	}
	
	virtual void tick(const float dt) = 0;
	
	virtual std::type_index typeIndex() = 0;
};

template <typename T>
struct ComponentMgr : ComponentMgrBase
{
	T * head = nullptr;
	T * tail = nullptr;
	
	virtual T * createComponent(const char * id) override final
	{
		T * component = new T();
		
		addComponent(component);
		
		return component;
	}
	
	virtual void addComponent(ComponentBase * in_component) override final
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

	virtual void destroyComponentImpl(ComponentBase * in_component) override final
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
		
		delete component;
	}
	
	virtual void tick(const float dt) override final
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
	
	virtual std::type_index typeIndex() override final
	{
		return std::type_index(typeid(T));
	}
};

struct ComponentSet
{
	ComponentBase * head = nullptr;
	
	void add(ComponentBase * component);
	void remove(ComponentBase * component);
	
	template <typename T>
	T * find()
	{
		for (auto * component = head; component != nullptr; component = component->next_in_set)
		{
			if (component->typeIndex() == std::type_index(typeid(T)))
				return static_cast<T*>(component);
		}
		
		return nullptr;
	}
	
	template <typename T>
	const T * find() const
	{
		for (auto * component = head; component != nullptr; component = component->next_in_set)
		{
			if (component->typeIndex() == std::type_index(typeid(T)))
				return static_cast<T*>(component);
		}
		
		return nullptr;
	}
};
