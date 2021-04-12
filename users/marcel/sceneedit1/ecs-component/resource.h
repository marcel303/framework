#pragma once

#include <typeindex>

struct ResourceBase
{
	virtual ~ResourceBase() = default;
	
	virtual std::type_index typeIndex() const = 0;
};

template <typename T>
struct Resource : ResourceBase
{
	virtual std::type_index typeIndex() const override final
	{
		return std::type_index(typeid(T));
	}
};

//

#include <string>

struct ResourcePtr
{
	std::string path;
	
	ResourceBase * getImpl(const std::type_index & typeIndex);
	
	template <typename T>
	T * get()
	{
		ResourceBase * resource = getImpl(std::type_index(typeid(T)));
		
		return static_cast<T*>(resource);
	}
};

//

struct ResourceDatabase
{
	struct Elem
	{
		static const int kMaxName = 64;
		
		Elem(const std::type_index & in_typeIndex)
			: typeIndex(in_typeIndex)
		{
		}
		
		std::type_index typeIndex;
		char name[kMaxName];
		ResourceBase * resource = nullptr;
		Elem * next = nullptr;
	};
	
	Elem * head = nullptr;

	~ResourceDatabase();
	
	void add(const char * name, ResourceBase * resource);
	void addComponentResource(const char * componentId, const char * resourceName, ResourceBase * resource);
	
	void remove(ResourceBase * resource);
	
	ResourceBase * find(const std::type_index & typeIndex, const char * name);
	
	template <typename T>
	T * find(const char * name)
	{
		ResourceBase * resource = find(std::type_index(typeid(T)), name);
		
		return static_cast<T*>(resource);
	}
};

//

extern ResourceDatabase g_resourceDatabase;

template <typename T>
T * findResource(const char * name)
{
	return g_resourceDatabase.find<T>(name);
}
