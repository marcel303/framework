#include "helpers.h"

// ecs-scene
#include "scene.h"
#include "sceneIo.h"
#include "sceneNodeComponent.h"
#include "template.h"
#include "templateIo.h"

// ecs-component
#include "componentType.h"

// libreflection-textio
#include "lineReader.h"
#include "lineWriter.h"
#include "reflection-textio.h"

// libgg
#include "Log.h"
#include "Path.h"
#include "StringEx.h"
#include "TextIO.h"

// std
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

// todo : add callback function for fetching templates. if set, use it as a fallback for when there is no inline template found

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
			LOG_ERR("failed to eat word");
			return false;
		}
		
		if (match_text(word, "template"))
		{
			const char * name;
			
			if (!eat_word_v2(line, name))
			{
				LOG_ERR("missing template name");
				return false;
			}
			
			// parse the template
			
			Template t;
			
			line_reader.push_indent();
			{
				if (!parseTemplateFromLines(line_reader, name, t))
				{
					LOG_ERR("failed to parse template");
					line_reader.pop_indent();
					return false;
				}
			}
			line_reader.pop_indent();
			
			// store it (by name) in a map for future reference
			
			templates.insert({ name, t });
		}
		else if (match_text(word, "entity"))
		{
			// note : entities are basically the same as templates, with the difference
			//        being that when we encounter an entity, we will instantiate it,
			//        rather than adding it to the collection of known templates
			
			const char * name;
			
			if (!eat_word_v2(line, name))
			{
				LOG_ERR("missing entity name");
				return false;
			}
			
			// parse the template
			
			Template t;
			
			line_reader.push_indent();
			{
				if (!parseTemplateFromLines(line_reader, nullptr, t))
				{
					LOG_ERR("failed to parse template (entity)");
					line_reader.pop_indent();
					return false;
				}
			}
			line_reader.pop_indent();
			
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
			// recursively apply overlays for entities
			
			// note : we defer applying overlays until we encounter the 'scene' section,
			//        as entities may depend on templates defined after their own definition
			
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
			
			for (auto & entity_itr : entities)
			{
				auto & t = entity_itr.second;
				
				if (!recursivelyOverlayBaseTemplates(
					t,
					true,
					true,
					fetchTemplate,
					&context))
				{
					LOG_ERR("failed to parse template (entity)");
					return false;
				}
				
				//dumpTemplateToLog(t);
			}
			
			// load/instantiate the scene objects
			
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
			LOG_ERR("failed to eat word");
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
	
	while ((line = (char*)line_reader.get_next_line(true, false)))
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
			LOG_ERR("failed to eat word");
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
			LOG_ERR("failed to instantiate components from template");
			
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

bool parseComponentFromLines(
	const TypeDB & typeDB,
	LineReader & line_reader,
	ComponentBase & out_component)
{
	auto * componentType = findComponentType(out_component.typeIndex());

	if (componentType == nullptr)
	{
		LOG_ERR("didn't find component type for component @ %p", &out_component);
		return false;
	}

	bool result = true;

	result &= object_fromlines_recursive(typeDB, componentType, &out_component, line_reader);

	return result;
}

//

bool writeSceneToLines(
	const TypeDB & typeDB,
	const Scene & scene,
	LineWriter & line_writer,
	const int in_indent)
{
	bool result = true;
	
	scene.assignAutoGeneratedNodeNames();
	
	int indent = in_indent;
	
	result &= writeSceneEntitiesToLines(typeDB, scene, line_writer, indent);
	
	//
	
	line_writer.append_indented_line(indent, "scene");
	
	indent++;
	{
		line_writer.append_indented_line(indent, "nodes");
		
		if (scene.rootNodeId != -1)
		{
			indent++;
			{
				auto & rootNode = scene.getRootNode();
				
				for (auto childNodeId : rootNode.childNodeIds)
				{
					// write node hierarchy
				
					result &= writeSceneNodeTreeToLines(
						scene,
						childNodeId,
						line_writer,
						indent);
				}
			}
			indent--;
		}
	}
	indent--;
	
	Assert(indent == 0);

	return result;
}

bool writeComponentToLines(
	const TypeDB & typeDB,
	const ComponentBase & component,
	LineWriter & line_writer,
	const int in_indent)
{
	auto * componentType = findComponentType(component.typeIndex());

	if (componentType == nullptr)
	{
		LOG_ERR("didn't find component type for component @ %p", &component);
		return false;
	}

	bool result = true;
	
	int indent = in_indent;
	
	char short_name[1024];
	
	result &= shrinkComponentTypeName(componentType->typeName, short_name, sizeof(short_name));
	
	line_writer.append_indented_line(indent, short_name);

	indent++;
	{
		result &= object_tolines_recursive(typeDB, componentType, &component, line_writer, indent);
	}
	indent--;
	
	return result;
}

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
			result &= writeComponentToLines(typeDB, *component, line_writer, indent);
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
