#include "Debugging.h"
#include "resource.h"
#include "StringEx.h"

//

ResourceDatabase g_resourceDatabase;

//

ResourceBase * ResourcePtr::getImpl(const std::type_index & typeIndex)
{
	if (path.empty())
		return nullptr;
	else
		return g_resourceDatabase.find(typeIndex, path.c_str());
}

//

ResourceDatabase::~ResourceDatabase()
{
	Elem * e = head;
	
	while (e != nullptr)
	{
		Elem * next = e->next;
		
		delete e->resource;
		e->resource = nullptr;
		
		delete e;
		
		e = next;
	}
}

void ResourceDatabase::add(const char * name, ResourceBase * resource)
{
	Elem * e = new Elem(resource->typeIndex());
	
	strcpy_s(e->name, sizeof(e->name), name);
	e->resource = resource;
	e->next = head;
	
	head = e;
}

void ResourceDatabase::addComponentResource(const char * componentId, const char * resourceName, ResourceBase * resource)
{
	std::string fullName = std::string(componentId) + "." + resourceName;
	
	add(fullName.c_str(), resource);
}

void ResourceDatabase::remove(ResourceBase * resource)
{
	for (Elem ** e = &head; *e != nullptr; e = &(*e)->next)
	{
		if ((*e)->resource == resource)
		{
			Elem * next = (*e)->next;
			
			delete *e;
			*e = next;
			
			return;
		}
	}
	
	AssertMsg(false, "failed to find resource to remove");
}

ResourceBase * ResourceDatabase::find(const std::type_index & typeIndex, const char * name)
{
	for (Elem * e = head; e != nullptr; e = e->next)
	{
		if (e->typeIndex == typeIndex && strcmp(e->name, name) == 0)
			return e->resource;
	}
	
	return nullptr;
}
