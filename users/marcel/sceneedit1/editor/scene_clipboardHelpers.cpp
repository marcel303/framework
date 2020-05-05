#include "component.h"
#include "componentType.h"
#include "helpers.h" // findComponentType
#include "helpers2.h" // g_typeDB
#include "lineReader.h"
#include "lineWriter.h"
#include "Log.h"
#include "reflection-textio.h"
#include "scene.h"
#include "scene_clipboardHelpers.h"
#include "TextIO.h"

bool node_to_text(const SceneNode & node, std::string & text)
{
	LineWriter line_writer;
	
	line_writer.append(":node\n");
	
	for (ComponentBase * component = node.components.head; component != nullptr; component = component->next_in_set)
	{
		auto * component_type = findComponentType(component->typeIndex());
		
		Assert(component_type != nullptr);
		if (component_type == nullptr)
			return false;
		
		line_writer.append_format("%s\n", component_type->typeName);
		
		if (object_tolines_recursive(g_typeDB, component_type, component, line_writer, 1) == false)
			continue;
	}
	
	text = line_writer.to_string();
	
	return true;
}

bool node_from_text(const char * text, SceneNode & node)
{
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (TextIO::loadText(text, lines, lineEndings) == false)
		return false;
	
	LineReader line_reader(lines, 0, 0);
	
	const char * id = line_reader.get_next_line(true);
	
	if (id == nullptr || (strcmp(id, ":node") != 0))
	{
		return false;
	}
	
	for (;;)
	{
		const char * component_type_name = line_reader.get_next_line(true);
		
		if (component_type_name == nullptr)
			break;
			
		if (component_type_name[0] == '\t')
		{
			// only one level of indentation may be added per line
			
			LOG_ERR("more than one level of indentation added on line %d", line_reader.get_current_line_index());
			return false;
		}
		
		auto * component_type = findComponentType(component_type_name);
		
		Assert(component_type != nullptr);
		if (component_type == nullptr)
		{
			node.freeComponents();
			return false;
		}
		
		auto * component = component_type->componentMgr->createComponent(node.components.id);
		
		line_reader.push_indent();
		{
			if (object_fromlines_recursive(g_typeDB, component_type, component, line_reader) == false)
			{
				node.freeComponents();
				return false;
			}
		}
		line_reader.pop_indent();
		
		node.components.add(component);
	}
	
	return true;
}
