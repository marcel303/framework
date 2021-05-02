#include "componentType.h"
#include "componentTypeDB.h"
#include "helpers.h"
#include "helpers2.h"
#include "reflection.h"

#include "AngleAxis.h"

// libgg
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

// libstdcpp
#include <string>

void registerBuiltinTypes(TypeDB & typeDB)
{
	typeDB.addPlain<bool>("bool", kDataType_Bool);
	typeDB.addPlain<int>("int", kDataType_Int);
	typeDB.addPlain<float>("float", kDataType_Float);
	typeDB.addPlain<Vec2>("vec2", kDataType_Float2);
	typeDB.addPlain<Vec3>("vec3", kDataType_Float3);
	typeDB.addPlain<Vec4>("vec4", kDataType_Float4);
	typeDB.addPlain<std::string>("string", kDataType_String);
	
	AngleAxis::reflect(typeDB);
}

void registerComponentTypes(TypeDB & typeDB, ComponentTypeDB & componentTypeDB)
{
	for (auto * registration = g_componentTypeRegistrationList; registration != nullptr; registration = registration->next)
	{
		componentTypeDB.registerComponentType(registration->createComponentType());
	}
	
	// perform reflection
	
	for (auto * componentType : componentTypeDB.componentTypes)
	{
		componentType->reflect(typeDB);
	}
}
