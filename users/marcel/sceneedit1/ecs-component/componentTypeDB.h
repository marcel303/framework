#pragma once

#include <typeindex>
#include <vector>

// forward declarations

struct ComponentTypeBase;

//

struct ComponentTypeDB
{
	void registerComponentType(ComponentTypeBase * componentType);

	bool initComponentMgrs();
	void shutComponentMgrs();

	ComponentTypeBase * findComponentType(const char * typeName) const;
	ComponentTypeBase * findComponentType(const std::type_index & typeIndex) const;

	template <typename T> T * findComponentType() const
	{
		return findComponentType(std::type_index(typeid(T)));
	}

	std::vector<ComponentTypeBase*> componentTypes;
};

extern ComponentTypeDB g_componentTypeDB;
