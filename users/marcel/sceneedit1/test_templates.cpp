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
		
		// todo : add support for a optional id directly after the type name
		
			const char * typeName = text;
			
			// apply conversion to the type name:
			// 'transform' becomes TransformComponent
			// 'rotate-transform' becomes RotateTransformComponent
			
		// todo : detect if the type name already has a 'Component' suffix
		
			char full_name[1024];
			int length = 0;
			bool capitalize = true;
			
			for (int i = 0; typeName[i] != 0 && length < 1024; ++i)
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
			
			// begin a new element, and set its type name to the full type name we just constructed
			
			out_template.components.emplace_back(TemplateComponent());
			
			current_component_element = &out_template.components.back();
			
			current_component_element->type_name = full_name;
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
			
			// is this the id for the component ?
			
			if (current_property_element->name == "id")
				current_component_element->id = current_property_element->value;
		}
	}
	
	return true;
}

static void instantiate()
{
		/*
			const ComponentTypeBase * componentType = findComponentType(full_name);
		 
			if (componentType == nullptr)
			{
				logError("unknown component type: %s (%s)", full_name, componentTypeName);
				//return false;
				continue; // fixme : this should be an error
			}
		*/
}

void test_templates()
{
	if (!framework.init(640, 480))
		return;

	registerComponentTypes();
	
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (!TextIO::load("textfiles/base-entity-v1.txt", lines, lineEndings))
		logError("failed to load text file");
	
	Template t;
	
	if (!parseTemplateFromLines(lines, t))
		logError("failed to parse template from lines");
	
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
