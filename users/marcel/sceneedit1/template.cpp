#include "Debugging.h"
#include "Log.h"
#include "Path.h"
#include "TextIO.h"

#include "component.h"
#include "componentType.h"
#include "helpers.h"
#include "lineReader.h"
#include "template.h"

#include <set>
#include <string.h>

bool parseTemplateFromLines(LineReader & line_reader, const char * name, Template & out_template)
{
	if (name != nullptr)
	{
		out_template.name = name;
	}
	
	// read components
	
	const char * line;
	
	while ((line = line_reader.get_next_line(true)))
	{
		if (line[0] == '\t')
		{
			// only one level of identation may be added per line
			
			LOG_ERR("more than one level of identation added one line %d", line_reader.get_current_line_index());
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
		
		bool appendSuffix = false;
		
		const char * suffix = "Component";
		
		if (strstr(typeName, suffix) == nullptr) // todo : check if the suffix is at the end of the type name
			appendSuffix = true;
		
		if (appendSuffix)
		{
			for (int i = 0; suffix[i] != 0 && length < 1024; ++i)
				full_name[length++] = suffix[i];
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
		
		current_component_element.type_name = full_name;
		current_component_element.id = id;
		
		if (current_component_element.type_name.empty())
		{
			LOG_ERR("found property outside the context of a component", 0);
			return false;
		}
	
		// read component properties
		
		line_reader.push_indent();
		{
			const char * propertyName;
			
			while ((propertyName = line_reader.get_next_line(true)))
			{
				// add a new property
				
				current_component_element.properties.emplace_back(TemplateComponentProperty());
				
				auto & current_property_element = current_component_element.properties.back();
				
			// todo : add support for adding a value directly after the property name
			
				current_property_element.name = propertyName;
				
				// read property value lines
				
				line_reader.push_indent();
				{
					const char * value_line;
					
					while ((value_line = line_reader.get_next_line(false)))
						current_property_element.value_lines.push_back(value_line);
				}
				line_reader.pop_indent();
			}
		}
		line_reader.pop_indent();
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
	
	LineReader line_reader(lines, 0, 0);
	
	if (!parseTemplateFromLines(line_reader, filename, t))
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

bool applyTemplateOverlaysWithCallback(const char * name, const Template & t, Template & out_template, const bool allowAddingComponentsFromBase, FetchTemplateCallback fetchTemplate, void * user_data)
{
	std::set<std::string> processed;
	std::vector<Template> templates;
	
	processed.insert(name);
	templates.push_back(t);
	
	std::string current_name = name;
	
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
		
		if (!fetchTemplate(current_name.c_str(), user_data, t))
		{
			LOG_ERR("failed to fetch template. name=%s", current_name.c_str());
			return false;
		}
		
		processed.insert(current_name);
		templates.emplace_back(std::move(t));
	}
	
	for (auto template_itr = templates.rbegin(); template_itr != templates.rend(); ++template_itr)
	{
		if (template_itr == templates.rbegin())
		{
			out_template = *template_itr;
		}
		else
		{
			if (!overlayTemplate(out_template, *template_itr, allowAddingComponentsFromBase, true))
			{
				LOG_ERR("failed to overlay template", 0);
				return false;
			}
		}
	}
	
	return true;
}

bool parseTemplateWithOverlaysWithCallback(const char * name, Template & out_template, const bool allowAddingComponentsFromBase, FetchTemplateCallback fetchTemplate, void * user_data)
{
	Template t;
	
	if (!fetchTemplate(name, user_data, t))
	{
		LOG_ERR("failed to fetch template. name=%s", name);
		return false;
	}
	
	return applyTemplateOverlaysWithCallback(name, t, out_template, allowAddingComponentsFromBase, fetchTemplate, user_data);
}

bool loadTemplateWithOverlaysFromFile(const char * path, Template & out_template, const bool allowAddingComponentsFromBase)
{
	auto directory = Path::GetDirectory(path);
	auto filename = Path::GetFileName(path);
	
	auto fetchTemplate = [](const char * name, void * user_data, Template & out_template) -> bool
	{
		const char * directory = (const char*)user_data;
		
		std::string path = std::string(directory) + "/" + name;
		
		if (!loadTemplateFromFile(path.c_str(), out_template))
		{
			LOG_ERR("failed to load template from file. path=%s", path.c_str());
			return false;
		}
		
		return true;
	};
	
	return parseTemplateWithOverlaysWithCallback(filename.c_str(), out_template, allowAddingComponentsFromBase, fetchTemplate, (void*)directory.c_str());
}

bool instantiateComponentsFromTemplate(const TypeDB & typeDB, const Template & t, ComponentSet & componentSet)
{
	for (auto & component_template : t.components)
	{
		const ComponentTypeBase * componentType = findComponentType(component_template.type_name.c_str());
		
		if (componentType == nullptr)
		{
			LOG_ERR("unknown component type: %s", component_template.type_name.c_str());
			return false;
		}
		
		ComponentBase * component = componentType->componentMgr->createComponent(component_template.id.c_str());
		
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
			
			LineReader line_reader(property_template.value_lines, 0, 0);
			
			if (member_fromlines_recursive(g_typeDB, member, component, line_reader) == false)
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
			component.type_name.c_str(),
			component.id.c_str());
		
		for (auto & property : component.properties)
		{
			LOG_DBG("%30s : %20s : %20s = %s",
				component.type_name.c_str(),
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
