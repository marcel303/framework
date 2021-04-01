#pragma once

#include "Debugging.h"
#include <stdlib.h> // realloc
#include <typeindex>

// forward declarations

struct ComponentBase;
struct ComponentMgrBase;
struct ComponentSet;

// component set ids

const int kComponentSetIdInvalid = -1;

int allocComponentSetId();
void freeComponentSetId(int & id);

// components

struct ComponentBase
{
	ComponentSet * componentSet = nullptr;
	ComponentBase * next_in_set = nullptr; // next component in the component set
	
	virtual ~ComponentBase();
	
	virtual void tick(const float dt) { }
	virtual bool init() { return true; }
	
	virtual std::type_index typeIndex() const = 0;
	
	virtual void propertyChanged(void * address) { };
	
	template <typename T>
	bool isType() const
	{
		return typeIndex() == std::type_index(typeid(T));
	}
};

template <typename T>
struct Component : ComponentBase
{
	T * next = nullptr;
	T * prev = nullptr;
	
	virtual std::type_index typeIndex() const override final
	{
		return std::type_index(typeid(T));
	}
};

struct ComponentMgrBase
{
	virtual ~ComponentMgrBase() { }
	
	virtual bool init() { return true; }
	virtual void shut() { }
	
	virtual ComponentBase * createComponent(const int id) = 0;
	virtual void destroyComponent(const int id) = 0;
	
	virtual ComponentBase * getComponent(const int id) = 0;
	
	virtual void tick(const float dt) = 0;
	
	virtual std::type_index typeIndex() const = 0;
};

template <typename T>
struct ComponentMgr : ComponentMgrBase
{
	T ** components = nullptr;
	int numComponents = 0;
	
	T * head = nullptr;
	T * tail = nullptr;
	
	virtual T * createComponent(const int id) override final
	{
		if (id + 1 > numComponents)
			resizeCapacity(id + 1);
		
		T * component = new T();
		
		Assert(components[id] == nullptr);
		components[id] = component;
		
		linkComponent(component);
		
		return component;
	}
	
	virtual void destroyComponent(const int id) override final
	{
		Assert(id >= 0 && id < numComponents && components[id] != nullptr);
		auto *& component = components[id];
		
		unlinkComponent(component);
		
		delete component;
		component = nullptr;
	}
	
	virtual T * getComponent(const int id) override final
	{
		Assert(id >= 0 && id < numComponents);
		auto * component = components[id];
		
		return component;
	}
	
	void resizeCapacity(const int capacity)
	{
		components = (T**)realloc(components, capacity * sizeof(T*));
		for (int i = numComponents; i < capacity; ++i)
			components[i] = nullptr;
		numComponents = capacity;
	}
	
	void linkComponent(T * component)
	{
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

	void unlinkComponent(T * component)
	{
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
	
	virtual std::type_index typeIndex() const override final
	{
		return std::type_index(typeid(T));
	}
};

// component set

struct ComponentSet
{
	ComponentBase * head = nullptr;
	
	int id = kComponentSetIdInvalid;
	
	ComponentSet();
	~ComponentSet();
	
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
	
	template <typename T>
	bool contains() const
	{
		for (auto * component = head; component != nullptr; component = component->next_in_set)
		{
			if (component->typeIndex() == std::type_index(typeid(T)))
				return true;
		}
		
		return false;
	}
};

// -- component set helper functions

void freeComponentsInComponentSet(ComponentSet & componentSet);
void freeComponentInComponentSet(ComponentSet & componentSet, ComponentBase *& component);
