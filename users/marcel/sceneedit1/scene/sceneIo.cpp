#include "helpers.h"
#include "lineReader.h"
#include "Log.h"
#include "Path.h"
#include "scene.h"
#include "sceneIo.h"
#include "sceneNodeComponent.h"
#include "StringEx.h"
#include "template.h"
#include "templateIo.h"
#include "TextIO.h"
#include <string.h>

extern SceneNodeComponentMgr s_sceneNodeComponentMgr;

static int calculateIndentationLevel(const char * line)
{
	int result = 0;
	
	while (line[result] == '\t')
		result++;
	
	return result;
}

static bool is_whitespace(const char c)
{
	return isspace(c);
}

static bool eat_word_v2(char *& line, const char *& word)
{
	while (*line != 0 && is_whitespace(*line) == true)
		line++;
	
	if (*line == 0)
		return false;
	
	const bool isQuoted = *line == '"';

	if (isQuoted)
	{
		line++;

		word = line;
		
		while (*line != 0 && *line != '"')
			line++;
	}
	else
	{
		word = line;
		
		while (*line != 0 && is_whitespace(*line) == false)
			line++;
	}
	
	if (line > word)
	{
		if (*line != 0)
		{
			*line = 0;
			line++;
		}
		
		return true;
	}
	else
	{
		return false;
	}
}

static bool match_text(const char * text, const char * other)
{
	return strcmp(text, other) == 0;
}

bool parseSceneFromLines(
	const TypeDB & typeDB,
	LineReader & line_reader,
	const char * basePath,
	Scene & out_scene)
{
	std::map<std::string, Template> templates;
	
	std::map<std::string, Template> entities;
	
	char * line; // fixme : make const char
	
	while ((line = (char*)line_reader.get_next_line(true)))
	{
		if (line[0] == '\t')
		{
			// only one level of indentation may be added per line
			
			LOG_ERR("more than one level of indentation added on line %d", line_reader.get_current_line_index());
			return false;
		}
		
		const char * word;

		if (!eat_word_v2(line, word))
		{
			LOG_ERR("failed to eat word", 0);
			return false;
		}
		
		if (match_text(word, "template"))
		{
			const char * name;
			
			if (!eat_word_v2(line, name))
			{
				LOG_ERR("missing template name", 0);
				return false;
			}
			
			// parse the template
			
			Template t;
			
			line_reader.push_indent();
			{
				if (!parseTemplateFromLines(line_reader, name, t))
				{
					LOG_ERR("failed to parse template", 0);
					line_reader.pop_indent();
					return false;
				}
			}
			line_reader.pop_indent();
			
			// store it (by name) in a map for future reference
			
			templates.insert({ name, t });
			
		#if false
			// recursively apply overlays
			
			struct FetchContext
			{
				std::string basePath;
				std::map<std::string, Template> * templates;
			};
			
			auto fetchTemplate = [](
				const char * name,
				const void * user_data,
				Template & out_template) -> bool
			{
				const FetchContext * context = (FetchContext*)user_data;
				
				auto template_itr = context->templates->find(name);
				
				if (template_itr != context->templates->end())
				{
					out_template = template_itr->second;
					
					return true;
				}
				else
				{
					char path[256];
					sprintf_s(path, sizeof(path),
						"%s/%s",
						context->basePath.empty()
							? "."
							: context->basePath.c_str(),
						name);
					
					return parseTemplateFromFile(path, out_template);
				}
			};
			
			FetchContext context;
			context.basePath = basePath;
			context.templates = &templates;
			
			if (!recursivelyOverlayBaseTemplates(
				t,
				true,
				true,
				fetchTemplate,
				&context))
			{
				LOG_ERR("failed to parse template", 0);
				return false;
			}
			
			//dumpTemplateToLog(t);
		#endif
		}
		else if (match_text(word, "entity"))
		{
			const char * name;
			
			if (!eat_word_v2(line, name))
			{
				LOG_ERR("missing entity name", 0);
				return false;
			}
			
			// parse the template
			
			Template t;
			
			line_reader.push_indent();
			{
				if (!parseTemplateFromLines(line_reader, nullptr, t))
				{
					LOG_ERR("failed to parse template (entity)", 0);
					line_reader.pop_indent();
					return false;
				}
			}
			line_reader.pop_indent();
			
			// recursively apply overlays
			
			struct FetchContext
			{
				std::string basePath;
				std::map<std::string, Template> * templates;
			};
			
			auto fetchTemplate = [](
				const char * name,
				const void * user_data,
				Template & out_template) -> bool
			{
				const FetchContext * context = (FetchContext*)user_data;
				
				auto template_itr = context->templates->find(name);
				
				if (template_itr != context->templates->end())
				{
					out_template = template_itr->second;
					
					return true;
				}
				else
				{
					// todo : the base path should be derived from the path of the template, and the global base path
					
					char path[256];
					sprintf_s(path, sizeof(path),
						"%s/%s",
						context->basePath.empty()
							? "."
							: context->basePath.c_str(),
						name);
					
					return parseTemplateFromFile(path, out_template);
				}
			};
			
			FetchContext context;
			context.basePath = basePath;
			context.templates = &templates;
			
			if (!recursivelyOverlayBaseTemplates(
				t,
				true,
				true,
				fetchTemplate,
				&context))
			{
				LOG_ERR("failed to parse template (entity)", 0);
				return false;
			}
			
			//dumpTemplateToLog(t);
			
			// check if an entity with the same name exists already
			
			if (entities.count(name) != 0)
			{
				LOG_ERR("entity with name '%s' already exists", name);
				return false;
			}
			
			// add the entity to the map
			
			entities.insert({ name, t });
		}
		else if (match_text(word, "scene"))
		{
			line_reader.push_indent();
			{
				if (!parseSceneObjectFromLines(
					typeDB,
					line_reader,
					out_scene,
					entities))
				{
					line_reader.pop_indent();
					return false;
				}
			}
			line_reader.pop_indent();
		}
		else
		{
			LOG_ERR("syntax error: %s", word);
			return false;
		}
	}
	
	return true;
}

bool parseSceneFromFile(
	const TypeDB & typeDB,
	const char * path,
	Scene & out_scene)
{
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;

	if (!TextIO::load(path, lines, lineEndings))
	{
		LOG_ERR("failed to load text file. path=%s", path);
		return false;
	}
	else
	{
		const std::string basePath = Path::GetDirectory(path);
		
		LineReader line_reader(lines, 0, 0);
		
		const bool result = parseSceneFromLines(
			typeDB,
			line_reader,
			basePath.c_str(),
			out_scene);
		
		if (result == false)
			line_reader.disable_dtor_check();

		return result;
	}
}

bool parseSceneObjectFromLines(
	const TypeDB & typeDB,
	LineReader & line_reader,
	Scene & out_scene,
	std::map<std::string, Template> & templates)
{
	char * line; // fixme : use const char
	
	while ((line = (char*)line_reader.get_next_line(true)))
	{
		if (line[0] == '\t')
		{
			// only one level of indentation may be added per line
			
			LOG_ERR("more than one level of indentation added on line %d", line_reader.get_current_line_index());
			return false;
		}
		
		//
		
		const char * word;
		
	// fixme : destructive operation!
	
		if (!eat_word_v2(line, word))
		{
			LOG_ERR("failed to eat word", 0);
			return false;
		}
		
		if (match_text(word, "nodes"))
		{
			line_reader.push_indent();
			{
				if (!parseSceneObjectStructureFromLines(typeDB, line_reader, out_scene, templates))
					return false;
			}
			line_reader.pop_indent();
		}
		else
		{
			LOG_ERR("syntax error on line %d", line_reader.get_current_line_index());
			return false;
		}
	}
	
	return true;
}

bool parseSceneObjectStructureFromLines(
	const TypeDB & typeDB,
	LineReader & line_reader,
	Scene & out_scene,
	std::map<std::string, Template> & templates)
{
	std::vector<SceneNode*> node_stack;
	
	node_stack.push_back(&out_scene.getRootNode());
	
	char * line; // fixme : make const char
	
	int current_level = -1;
	
	while ((line = (char*)line_reader.get_next_line(true)))
	{
		const int new_level = calculateIndentationLevel(line);
		
		if (new_level > current_level + 1)
		{
			// only one level of indentation may be added per line
			
			LOG_ERR("more than one level of indentation added on line %d", line_reader.get_current_line_index());
			return false;
		}
		
		//
		
		while (current_level >= new_level)
		{
			node_stack.pop_back();
			
			--current_level;
		}
		
		current_level = new_level;

		
		//
		
		const char * name;
		
		if (!eat_word_v2(line, name))
		{
			LOG_ERR("failed to eat word", 0);
			return false;
		}
		
		auto template_itr = templates.find(name);
		
		if (template_itr == templates.end())
		{
			LOG_ERR("entity does not exist: %s", name);
			return false;
		}
		
		auto & t = template_itr->second;
		
		SceneNode * node = new SceneNode();
		node->name = name;
		node->id = out_scene.allocNodeId();
		node->parentId = node_stack.back()->id;
		
		if (!instantiateComponentsFromTemplate(typeDB, t, node->components))
		{
			LOG_ERR("failed to instantiate components from template", 0);
			
			node->freeComponents();
			
			delete node;
			node = nullptr;
			
			return false;
		}
		
		if (node->components.contains<SceneNodeComponent>() == false)
		{
			auto * sceneNodeComponent = s_sceneNodeComponentMgr.createComponent(node->components.id);
			sceneNodeComponent->name = node->name;
			node->components.add(sceneNodeComponent);
		}
		
		auto parentNode_itr = out_scene.nodes.find(node->parentId);
		Assert(parentNode_itr != out_scene.nodes.end());
		if (parentNode_itr != out_scene.nodes.end())
		{
			auto & parentNode = parentNode_itr->second;
			parentNode->childNodeIds.push_back(node->id);
		}
		
		out_scene.nodes.insert({ node->id, node });
		
		node_stack.push_back(node);
	}
	
	return true;
}

//

// ecs-component
#include "componentType.h"

// libreflection-textio
#include "lineWriter.h"
#include "reflection-textio.h"

bool writeSceneEntityToLines(
	const TypeDB & typeDB,
	const SceneNode & node,
	LineWriter & line_writer,
	const int in_indent)
{
	bool result = true;
	
	int indent = in_indent;
	
	char node_definition[128];
	sprintf(node_definition, "entity %s", node.name.c_str());
	line_writer.append_indented_line(indent, node_definition);
	
	indent++;
	{
		for (auto * component = node.components.head; component != nullptr; component = component->next_in_set)
		{
			auto * componentType = findComponentType(component->typeIndex());
			
			if (componentType == nullptr)
			{
				LOG_ERR("didn't find component type for component @ %p", component);
				result &= false;
				continue;
			}
			
			// check if the component type name ends with 'Component'. in this case
			// we'll want to write a short hand component name which is easier
			// to hand-edit
			
			const char * suffix = strstr(componentType->typeName, "Component");
			const int suffix_length = 9;
			
			if (suffix != nullptr && suffix[suffix_length] == 0)
			{
				// make a short version of the component type name
				// e.g. RotateTransformComponent becomes 'rotate-transform'
				
				char short_name[1024];
				int length = 0;
				
				for (int i = 0; componentType->typeName + i < suffix && length < 1024; ++i)
				{
					if (isupper(componentType->typeName[i]))
					{
						if (i != 0)
							short_name[length++] = '-';
						
						short_name[length++] = tolower(componentType->typeName[i]);
					}
					else
						short_name[length++] = componentType->typeName[i];
				}
				
				if (length == 1024)
				{
					LOG_ERR("component type name too long: %s", componentType->typeName);
					result &= false;
				}
				else
				{
					short_name[length++] = 0;
					
					line_writer.append_indented_line(indent, short_name);
				}
			}
			else
			{
				// write the full name if the type name doesn't match the pattern
				
				LOG_WRN("writing full component type name. this is unexpected. typeName=%s", componentType->typeName);
				line_writer.append_indented_line(indent, componentType->typeName);
			}
			
			indent++;
			{
				result &= object_tolines_recursive(typeDB, componentType, component, line_writer, indent);
			}
			indent--;
		}
	}
	indent--;
	
	return result;
}

bool writeSceneEntitiesToLines(
	const TypeDB & typeDB,
	const Scene & scene,
	LineWriter & line_writer,
	const int in_indent)
{
	bool result = true;
	
	int indent = in_indent;
	
	for (auto & node_itr : scene.nodes)
	{
		auto & node = *node_itr.second;
		
		writeSceneEntityToLines(typeDB, node, line_writer, indent);
		
		/*
		char node_definition[128];
		sprintf(node_definition, "entity %s", node->name.c_str());
		line_writer.append_indented_line(indent, node_definition);
		
		indent++;
		{
			for (auto * component = node->components.head; component != nullptr; component = component->next_in_set)
			{
				auto * componentType = findComponentType(component->typeIndex());
				
				if (componentType == nullptr)
				{
					LOG_ERR("didn't find component type for component @ %p", component);
					result &= false;
					continue;
				}
				
				// check if the component type name ends with 'Component'. in this case
				// we'll want to write a short hand component name which is easier
				// to hand-edit
				
				const char * suffix = strstr(componentType->typeName, "Component");
				const int suffix_length = 9;
				
				if (suffix != nullptr && suffix[suffix_length] == 0)
				{
					// make a short version of the component type name
					// e.g. RotateTransformComponent becomes 'rotate-transform'
					
					char short_name[1024];
					int length = 0;
					
					for (int i = 0; componentType->typeName + i < suffix && length < 1024; ++i)
					{
						if (isupper(componentType->typeName[i]))
						{
							if (i != 0)
								short_name[length++] = '-';
							
							short_name[length++] = tolower(componentType->typeName[i]);
						}
						else
							short_name[length++] = componentType->typeName[i];
					}
					
					if (length == 1024)
					{
						LOG_ERR("component type name too long: %s", componentType->typeName);
						result &= false;
					}
					else
					{
						short_name[length++] = 0;
						
						line_writer.append_indented_line(indent, short_name);
					}
				}
				else
				{
					// write the full name if the type name doesn't match the pattern
					
					LOG_WRN("writing full component type name. this is unexpected. typeName=%s", componentType->typeName);
					line_writer.append_indented_line(indent, componentType->typeName);
				}
				
				indent++;
				{
					result &= object_tolines_recursive(typeDB, componentType, component, line_writer, indent);
				}
				indent--;
			}
		}
		indent--;
		*/
		
		line_writer.append('\n');
	}
	
	return result;
}

bool writeSceneNodeTreeToLines(
	const Scene & scene,
	const int rootNodeId,
	LineWriter & line_writer,
	const int indent)
{
	bool result = true;
	
	auto & node = scene.getNode(rootNodeId);
	
	line_writer.append_indented_line(indent, node.name.c_str());
	
	for (auto childNodeId : node.childNodeIds)
	{
		result &= writeSceneNodeTreeToLines(scene, childNodeId, line_writer, indent + 1);
	}
	
	return result;
}
