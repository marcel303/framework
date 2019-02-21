#include "Debugging.h"
#include "Log.h"
#include "TextIO.h"

#include "component.h"
#include "componentType.h"
#include "helpers.h"
#include "template.h"

static bool isEmptyLine(const char * line)
{
	for (int i = 0; line[i] != 0; ++i)
		if (!isspace(line[i]))
			return false;
	
	return true;
}

static int calculateIndentationLevel(const char * line)
{
	int result = 0;
	
	while (line[result] == '\t')
		result++;
	
	return result;
}

bool parseTemplateFromLines(const std::vector<std::string> & lines, Template & out_template)
{
	TemplateComponent * current_component_element = nullptr;
	TemplateComponentProperty * current_property_element = nullptr;
	
	int current_level = -1;
	
	for (auto & line : lines)
	{
		if (isEmptyLine(line.c_str()))
			continue;
		
		const int new_level = calculateIndentationLevel(line.c_str());
		
		if (new_level > current_level + 1)
		{
			// only one level of identation may be added per line
			
			LOG_ERR("syntax error", 0);
			return false;
		}
		
		if (new_level < 2)
			current_property_element = nullptr;
		if (new_level < 1)
			current_component_element = nullptr;
		
		if (current_component_element == nullptr)
			Assert(current_property_element == nullptr);
		
		current_level = new_level;
		
		const char * text = line.c_str() + current_level;
		
		if (current_level == 0)
		{
			if (memcmp(text, "base", 4) == 0)
			{
				// base specifier. find the base name at the end
			
				const char * base = text;
				
				while (base[0] != 0 && !isspace(base[0]))
					base++;
				
				while (base[0] != 0 && isspace(base[0]))
					base++;
				
				if (base[0] == 0)
				{
					LOG_ERR("missing base name", 0);
					return false;
				}
				
				out_template.base = base;
				
				continue;
			}
			
			// type name
			
			Assert(current_component_element == nullptr);
			
			const char * typeName = text;
			
			// apply conversion to the type name:
			// 'transform' becomes TransformComponent
			// 'rotate-transform' becomes RotateTransformComponent
			
		// todo : detect if the type name already has a 'Component' suffix
		
			char full_name[1024];
			int length = 0;
			bool capitalize = true;
			
			for (int i = 0; typeName[i] != 0 && !isspace(typeName[i]) && length < 1024; ++i)
			{
				if (typeName[i] == '-')
					capitalize = true;
				else if (capitalize)
				{
					capitalize = false;
					full_name[length++] = toupper(typeName[i]);
				}
				else
					full_name[length++] = typeName[i];
			}
			
			const char * suffix = "Component";
			
			for (int i = 0; suffix[i] != 0 && length < 1024; ++i)
				full_name[length++] = suffix[i];
			
			if (length < 1024)
				full_name[length++] = 0;
			
			if (length == 1024)
			{
				LOG_ERR("type name is too long", 0);
				return false;
			}
			
			// find the optional id at the end
			
			const char * id = typeName;
			
			while (id[0] != 0 && !isspace(id[0]))
				id++;
			
			while (id[0] != 0 && isspace(id[0]))
				id++;
			
			// begin a new element, and set its type name to the full type name we just constructed
			
			out_template.components.emplace_back(TemplateComponent());
			
			current_component_element = &out_template.components.back();
			
			current_component_element->type_name = full_name;
			current_component_element->id = id;
		}
		else if (current_level == 1)
		{
			// property name
			
			Assert(current_component_element != nullptr);
			Assert(current_property_element == nullptr);
			
			if (current_component_element->type_name.empty())
			{
				LOG_ERR("found property outside the context of a component", 0);
				return false;
			}
			
			// begin a new property
			
			current_component_element->properties.emplace_back(TemplateComponentProperty());
			
			current_property_element = &current_component_element->properties.back();
			
			const char * propertyName = text;
			
		// todo : add support for adding a value directly after the property name
		
			current_property_element->name = propertyName;
		}
		else if (current_level == 2)
		{
			// property value
			
			Assert(current_component_element != nullptr);
			Assert(current_property_element != nullptr);
			
			const char * propertyValue = text;
			
			current_property_element->value = propertyValue;
		}
	}
	
	return true;
}

bool loadTemplateFromFile(const char * filename, Template & t)
{
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (!TextIO::load(filename, lines, lineEndings))
	{
		LOG_ERR("failed to load text file", 0);
		return false;
	}
	
	if (!parseTemplateFromLines(lines, t))
	{
		LOG_ERR("failed to parse template from lines", 0);
		return false;
	}
	
	return true;
}

bool overlayTemplate(Template & target, const Template & overlay, const bool allowAddingComponents, const bool allowAddingProperties)
{
	for (auto & overlay_component : overlay.components)
	{
		TemplateComponent * target_component = nullptr;
		
		for (auto & target_component_itr : target.components)
		{
			if (target_component_itr.type_name == overlay_component.type_name &&
				target_component_itr.id == overlay_component.id)
				target_component = &target_component_itr;
		}
		
		if (target_component == nullptr)
		{
			if (allowAddingComponents == false)
			{
				LOG_ERR("component doesn't exist: %s, id=%s",
					overlay_component.type_name.c_str(),
					overlay_component.id.c_str());
				return false;
			}
			else
			{
				// component doesn't exist yet. add it
				
				target.components.push_back(overlay_component);
			}
		}
		else
		{
			// component already exists. overlay properties
			
			for (auto & overlay_property : overlay_component.properties)
			{
				TemplateComponentProperty * target_property = nullptr;
				
				for (auto & target_property_itr : target_component->properties)
				{
					if (target_property_itr.name == overlay_property.name)
						target_property = &target_property_itr;
				}
				
				if (target_property == nullptr)
				{
					if (allowAddingProperties == false)
					{
						LOG_ERR("component property doesn't exist: component=%s, id=%s, property=%s",
							overlay_component.type_name.c_str(),
							overlay_component.id.c_str(),
							overlay_property.name.c_str());
						return false;
					}
					else
					{
						// property doesn't exist yet. add it
						
						target_component->properties.push_back(overlay_property);
					}
				}
				else
				{
					// property already exists. overlay value
					
					target_property->value = overlay_property.value;
				}
			}
		}
	}
	
	return true;
}

bool instantiateComponentsFromTemplate(const Template & t, ComponentSet & componentSet)
{
	for (auto & component_template : t.components)
	{
		const ComponentTypeBase * componentType = findComponentType(component_template.type_name.c_str());
		
		if (componentType == nullptr)
		{
			LOG_ERR("unknown component type: %s", component_template.type_name.c_str());
			return false;
		}
		
		ComponentBase * component = componentType->componentMgr->createComponent();
		
		for (auto & property_template : component_template.properties)
		{
			ComponentPropertyBase * property = nullptr;
			
			for (auto & property_itr : componentType->properties)
				if (property_itr->name == property_template.name)
					property = property_itr;
			
			if (property == nullptr)
			{
				LOG_ERR("unknown property: %s", property_template.name.c_str());
				return false; // fixme : leaks
			}
			
			property->from_text(component, property_template.value.c_str());
		}
		
		componentSet.add(component);
	}
	
	return true;
}
