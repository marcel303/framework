#pragma once

#include <typeindex>
#include <vector>

// forward declarations

struct ComponentTypeBase;

// component type DB

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

// component type registration list

struct ComponentTypeRegistrationBase
{
	ComponentTypeRegistrationBase * next = nullptr;
	
	virtual ComponentTypeBase * createComponentType() = 0;
};

extern ComponentTypeRegistrationBase * g_componentTypeRegistrationList;

#define ComponentTypeRegistration(type) \
	struct ComponentTypeRegistration_ ## type : ComponentTypeRegistrationBase \
	{ \
		ComponentTypeRegistration_ ## type() \
		{ \
			next = g_componentTypeRegistrationList; \
			g_componentTypeRegistrationList = this; \
		} \
		virtual ComponentTypeBase * createComponentType() override final \
		{ \
			return new type(); \
		} \
	}; \
	extern ComponentTypeRegistration_ ## type g_ComponentTypeRegistration_ ## type; \
	ComponentTypeRegistration_ ## type g_ComponentTypeRegistration_ ## type;
