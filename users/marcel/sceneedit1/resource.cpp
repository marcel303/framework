#include "resource.h"
#include "StringEx.h"

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

ResourceBase * ResourceDatabase::find(const std::type_index & typeIndex, const char * name)
{
	for (Elem * e = head; e != nullptr; e = e->next)
	{
		if (e->typeIndex == typeIndex && strcmp(e->name, name) == 0)
			return e->resource;
	}
	
	return nullptr;
}
