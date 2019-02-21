#include "framework.h"
#include "TextIO.h"

#include "component.h"
#include "componentType.h"
#include "helpers.h"

#include <string>
#include <string.h>

struct TemplateComponentProperty
{
	std::string name;
	std::string value; // todo : make this a list of values. with corrected indentation. leave it the the property type to deserialize itself
};

struct TemplateComponent
{
	std::string type_name;
	std::string id;
	
	std::vector<TemplateComponentProperty> properties;
};

struct Template
{
	std::vector<TemplateComponent> components;
};

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

static bool parseTemplateFromLines(const std::vector<std::string> & lines, Template & out_template)
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
			
			logError("syntax error");
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
				logError("type name is too long");
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
				logError("found property outside the context of a component");
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

static bool overlayTemplate(Template & target, const Template & overlay, const bool allowAddingComponents, const bool allowAddingProperties)
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
				logError("component doesn't exist: %s, id=%s",
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
						logError("component property doesn't exist: component=%s, id=%s, property=%s",
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

static bool instantiateComponentsFromTemplate(const Template & t)
{
	for (auto & component_template : t.components)
	{
		const ComponentTypeBase * componentType = findComponentType(component_template.type_name.c_str());
		
		if (componentType == nullptr)
		{
			logError("unknown component type: %s", component_template.type_name.c_str());
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
				logError("unknown property: %s", property_template.name.c_str());
				return false; // fixme : leaks
			}
			
			property->from_text(component, property_template.value.c_str());
		}
		
		componentType->componentMgr->removeComponent(component);
	}
	
	return true;
}

static bool loadTemplateFromFile(const char * filename, Template & t)
{
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (!TextIO::load(filename, lines, lineEndings))
	{
		logError("failed to load text file");
		return false;
	}
	
	if (!parseTemplateFromLines(lines, t))
	{
		logError("failed to parse template from lines");
		return false;
	}
	
	return true;
}

void test_templates()
{
	if (!framework.init(640, 480))
		return;

	registerComponentTypes();
	
	Template t;
	
	if (!loadTemplateFromFile("textfiles/base-entity-v1.txt", t))
		logError("failed to load template from file");
	
	Template overlay;
	
	if (!loadTemplateFromFile("textfiles/base-entity-v1-overlay.txt", overlay))
		logError("failed to load template from file");
	
	if (!overlayTemplate(t, overlay, false, true))
		logError("failed to overlay template");
	
	if (!instantiateComponentsFromTemplate(t))
		logError("failed to instantiate components from template");
	
	// show what we just parsed
	
	for (auto & component : t.components)
	{
		for (auto & property : component.properties)
		{
			logDebug("%30s : %20s : %20s = %s",
				component.type_name.c_str(),
				component.id.c_str(),
				property.name.c_str(),
				property.value.c_str());
		}
	}
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;

		framework.beginDraw(0, 0, 0, 0);
		{

		}
		framework.endDraw();
	}

	framework.shutdown();
}
