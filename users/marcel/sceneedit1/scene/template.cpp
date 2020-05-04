#include "template.h"

#include "component.h"
#include "componentType.h"
#include "helpers.h"
#include "lineReader.h"

#include "Log.h"

bool instantiateComponentsFromTemplate(
	const TypeDB & typeDB,
	const Template & t,
	ComponentSet & componentSet)
{
	for (auto & component_template : t.components)
	{
		const ComponentTypeBase * componentType = findComponentType(component_template.typeName.c_str());
		
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
				return false;
			}
			
			LineReader lineReader(property_template.value_lines, 0, 0);
			
			if (member_fromlines_recursive(g_typeDB, member, component, lineReader) == false)
			{
				LOG_ERR("failed to deserialize property from text: property=%s, lines=", property_template.name.c_str());
				for (auto & line : property_template.value_lines)
					LOG_ERR("%s", line.c_str());
				return false;
			}
		}
		
		componentSet.add(component);
	}
	
	return true;
}

void dumpTemplateToLog(const Template & t)
{
	for (auto & component : t.components)
	{
		LOG_DBG("%30s : %20s *",
			component.typeName.c_str(),
			component.id.c_str());
		
		for (auto & property : component.properties)
		{
			LOG_DBG("%30s : %20s : %20s = %s",
				component.typeName.c_str(),
				component.id.c_str(),
				property.name.c_str(),
				property.value_lines.size() == 1 ? property.value_lines[0].c_str() : "");
			
			if (property.value_lines.size() > 1)
			{
				for (auto & line : property.value_lines)
					LOG_DBG("\t%s", line.c_str());
			}
		}
	}
}
