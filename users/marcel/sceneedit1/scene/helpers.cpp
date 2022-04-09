#include "componentType.h"
#include "componentTypeDB.h"
#include "helpers.h"
#include "template.h"

#include "lineReader.h"
#include "reflection-textio.h"

#include "Log.h"

#include <string.h> // strcmp

bool instantiateComponentsFromTemplate(
	const TypeDB & typeDB,
	const Template & t,
	ComponentSet & componentSet)
{
	for (auto & component_template : t.components)
	{
		const ComponentTypeBase * componentType = g_componentTypeDB.findComponentType(component_template.typeName.c_str());
		
		if (componentType == nullptr)
		{
			LOG_ERR("unknown component type: %s", component_template.typeName.c_str());
			return false;
		}
		
		ComponentBase * component = componentType->componentMgr->createComponent(componentSet.id);
		
		for (auto & property_template : component_template.properties)
		{
			Member * member = nullptr;
			
			for (auto * member_itr = componentType->members_head; member_itr != nullptr; member_itr = member_itr->next)
				if (strcmp(member_itr->name, property_template.name.c_str()) == 0)
					member = member_itr;
			
			if (member == nullptr)
			{
				LOG_ERR("unknown property: %s", property_template.name.c_str());
				componentType->componentMgr->destroyComponent(componentSet.id);
				return false;
			}
			
			LineReader lineReader(&property_template.value_lines, 0, 0);
			
			if (member_fromlines_recursive(typeDB, member, component, lineReader) == false)
			{
				LOG_ERR("failed to deserialize property from text: property=%s, lines=", property_template.name.c_str());
				for (auto & line : property_template.value_lines)
					LOG_ERR("%s", line.c_str());
				componentType->componentMgr->destroyComponent(componentSet.id);
				return false;
			}
		}
		
		componentSet.add(component);
	}
	
	return true;
}
