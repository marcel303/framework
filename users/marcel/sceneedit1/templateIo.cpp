#include "templateIo.h"

#include "lineReader.h"
#include "template.h"

#include "Debugging.h"
#include "Log.h"
#include "Path.h"
#include "TextIO.h"

#include <set>
#include <string.h>

bool parseTemplateFromLines(
	LineReader & lineReader,
	const char * name,
	Template & out_template)
{
	if (name != nullptr)
	{
		out_template.name = name;
	}
	
	// read components
	
	const char * line;
	
	while ((line = lineReader.get_next_line(true)))
	{
		if (line[0] == '\t')
		{
			// only one level of indentation may be added per line
			
			LOG_ERR("more than one level of indentation added on line %d", lineReader.get_current_line_index());
			return false;
		}
		
		if (memcmp(line, "base", 4) == 0)
		{
			// special : base specifier. find the base name at the end
		
			const char * base = line;
			
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
		
		// component type name
		
		const char * typeName = line;
		
		// apply conversion to the type name:
		// 'transform' becomes TransformComponent
		// 'rotate-transform' becomes RotateTransformComponent
		// 'TransformComponent' remains TransformComponent, due to it ending with 'Component'
		
		bool hasComponentSuffix = false;
		
		const char * componentSuffix = "Component";
		
		// detect if the type name already has a 'Component' suffix
		
		if (strstr(typeName, componentSuffix) != nullptr)
			hasComponentSuffix = true;
	
		char full_name[1024];
		int length = 0;
		
		// if the type name has a 'Component' suffix, just keep the type name
		// otherwise, apply conversion
		
		if (hasComponentSuffix)
		{
			for (int i = 0; typeName[i] != 0 && !isspace(typeName[i]) && length < 1024; ++i)
				full_name[length++] = typeName[i];
		}
		else
		{
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
		
			for (int i = 0; componentSuffix[i] != 0 && length < 1024; ++i)
				full_name[length++] = componentSuffix[i];
		}
		
		if (length < 1024)
			full_name[length++] = 0;
		
		if (length == 1024)
		{
			LOG_ERR("type name is too long", 0);
			return false;
		}
		
		// find the optional component id at the end
		
		const char * id = typeName;
		
		while (id[0] != 0 && !isspace(id[0]))
			id++;
		
		while (id[0] != 0 && isspace(id[0]))
			id++;
		
		// add a new component element, and set its type name to the full type name we just constructed
		
		out_template.components.emplace_back(TemplateComponent());
		
		auto & current_component_element = out_template.components.back();
		
		current_component_element.typeName = full_name;
		current_component_element.id = id;
		
		if (current_component_element.typeName.empty())
		{
			LOG_ERR("found property outside the context of a component", 0);
			return false;
		}
	
		// read component properties
		
		lineReader.push_indent();
		{
			const char * propertyName;
			
			while ((propertyName = lineReader.get_next_line(true)))
			{
				if (propertyName[0] == '\t')
				{
					// only one level of indentation may be added per line
					
					LOG_ERR("more than one level of indentation added on line %d", lineReader.get_current_line_index());
					return false;
				}
				
				// add a new property
				
				current_component_element.properties.emplace_back(TemplateComponentProperty());
				
				auto & current_property_element = current_component_element.properties.back();
				
			// todo : add support for adding a value directly after the property name
			
				current_property_element.name = propertyName;
				
				// read property value lines
				
				lineReader.push_indent();
				{
					const char * value_line;
					
					while ((value_line = lineReader.get_next_line(false)))
					{
						// note : we don't check if value_line[0] == '\t' here, as we are simply extracting line, not parsing them (yet)
						
						current_property_element.value_lines.push_back(value_line);
					}
				}
				lineReader.pop_indent();
			}
		}
		lineReader.pop_indent();
	}
	
	return true;
}

bool parseTemplateFromFile(const char * path, Template & out_template)
{
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (!TextIO::load(path, lines, lineEndings))
	{
		LOG_ERR("failed to load text file", 0);
		return false;
	}
	
	LineReader lineReader(lines, 0, 0);
	
	if (!parseTemplateFromLines(lineReader, path, out_template))
	{
		LOG_ERR("failed to parse template from lines", 0);
		return false;
	}
	
	return true;
}

bool overlayTemplate(
	Template & target,
	const Template & overlay,
	const bool allowAddingComponents,
	const bool allowAddingProperties)
{
	for (auto & overlay_component : overlay.components)
	{
		TemplateComponent * target_component = nullptr;
		
		for (auto & target_component_itr : target.components)
		{
			if (target_component_itr.typeName == overlay_component.typeName &&
				target_component_itr.id == overlay_component.id)
				target_component = &target_component_itr;
		}
		
		if (target_component == nullptr)
		{
			if (allowAddingComponents == false)
			{
				LOG_ERR("component doesn't exist: %s, id=%s",
					overlay_component.typeName.c_str(),
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
							overlay_component.typeName.c_str(),
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
					
				// todo : add a more intelligent overlay, where individual structured members are overlaid
				// note : this will be A LOT of effort to make work. also, vectors will complicate things a lot
				//        we'll probably need to (temporarily) assign unique ids to each member
					target_property->value_lines = overlay_property.value_lines;
				}
			}
		}
	}
	
	return true;
}

bool recursivelyOverlayBaseTemplates(
	Template & t,
	const bool allowAddingComponents,
	const bool allowAddingProperties,
	const FetchTemplateCallback fetchTemplate,
	const void * userData)
{
	std::set<std::string> processed;
	std::vector<Template> templates;
	
	processed.insert(t.name);
	
	std::string current_name = t.name;
	
	for (;;)
	{
		// todo : let us inherit the path component of the previous name.. so we end up with the full relative path
	
		auto & new_name = templates.empty() ? t.base : templates.back().base;
		
		if (new_name.empty())
			break;
		
		if (processed.count(new_name) != 0)
		{
			LOG_ERR("cyclic dependency found. %s references %s which is already processed",
				current_name.c_str(),
				new_name.c_str());
			return false;
		}
		
		current_name = new_name;
		
		Template t;
		
		if (!fetchTemplate(current_name.c_str(), userData, t))
		{
			LOG_ERR("failed to fetch template. name=%s", current_name.c_str());
			return false;
		}
		
		processed.insert(current_name);
		templates.emplace_back(std::move(t));
	}
	
	for (auto template_itr = templates.rbegin(); template_itr != templates.rend(); ++template_itr)
	{
		if (!overlayTemplate(
			t,
			*template_itr,
			allowAddingComponents,
			allowAddingProperties))
		{
			LOG_ERR("failed to overlay template", 0);
			return false;
		}
	}
	
	return true;
}

bool parseTemplateFromCallbackAndRecursivelyOverlayBaseTemplates(
	const char * name,
	const bool allowAddingComponents,
	const bool allowAddingProperties,
	const FetchTemplateCallback fetchTemplate,
	const void * userData,
	Template & out_template)
{
	if (!fetchTemplate(name, userData, out_template))
	{
		LOG_ERR("failed to fetch template. name=%s", name);
		return false;
	}
	
	return recursivelyOverlayBaseTemplates(
		out_template,
		allowAddingComponents,
		allowAddingProperties,
		fetchTemplate,
		userData);
}

bool parseTemplateFromFileAndRecursivelyOverlayBaseTemplates(
	const char * path,
	const bool allowAddingComponents,
	const bool allowAddingProperties,
	Template & out_template)
{
	auto directory = Path::GetDirectory(path);
	auto filename = Path::GetFileName(path);
	
	auto fetchTemplate = [](
		const char * name,
		const void * userData,
		Template & out_template) -> bool
	{
		const char * directory = (const char*)userData;
		
		const std::string path =
			directory[0] == 0
			? name
			: std::string(directory) + "/" + name;
		
		if (!parseTemplateFromFile(path.c_str(), out_template))
		{
			LOG_ERR("failed to load template from file. path=%s", path.c_str());
			return false;
		}
		
		return true;
	};
	
	return parseTemplateFromCallbackAndRecursivelyOverlayBaseTemplates(
		filename.c_str(),
		allowAddingComponents,
		allowAddingProperties,
		fetchTemplate,
		(void*)directory.c_str(),
		out_template);
}