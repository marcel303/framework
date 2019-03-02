#include "lineReader.h"
#include "Log.h"
#include "scene.h"
#include "scene_fromText.h"
#include "sceneNodeComponent.h"
#include <string.h>

static bool isEmptyLineOrComment(const char * line)
{
	for (int i = 0; line[i] != 0; ++i)
	{
		if (line[i] == '#')
			return true;
		
		if (!isspace(line[i]))
			return false;
	}
	
	return true;
}

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

static void extractLinesGivenIndentationLevel(const std::vector<std::string> & lines, size_t & index, const int level, std::vector<std::string> & out_lines, const bool subtract_indentation)
{
	for (;;)
	{
		if (index == lines.size())
			break;
		
		const std::string & line = lines[index];
		
		if (isEmptyLineOrComment(line.c_str()))
		{
			++index;
			continue;
		}
		
		const int current_level = calculateIndentationLevel(line.c_str());
		
		if (current_level < level)
			break;
		
		if (subtract_indentation)
			out_lines.push_back(line.substr(level));
		else
			out_lines.push_back(line);
		
		++index;
	}
}

bool parseSceneFromLines(const TypeDB & typeDB, std::vector<std::string> & lines, Scene & out_scene)
{
	std::map<std::string, Template> templates;
	
	std::map<std::string, Template> entities;
	
	LineReader line_reader(lines, 0, 0);
	
	char * line; // fixme : make const char
	
	while ((line = (char*)line_reader.get_next_line(true)))
	{
		if (line[0] == '\t')
		{
			// only one level of identation may be added per line
			
			LOG_ERR("more than one level of identation added one line %d", line_reader.line_index);
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
			
			// recursively apply overlays
			
			struct FetchContext
			{
				std::map<std::string, Template> * templates;
			};
			
			auto fetchTemplate = [](const char * name, void * user_data, Template & out_template) -> bool
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
					return loadTemplateFromFile(name, out_template);
				}
			};
			
			FetchContext context;
			context.templates = &templates;
			
			if (!applyTemplateOverlaysWithCallback(name, t, t, true, fetchTemplate, &context))
			{
				LOG_ERR("failed to parse template", 0);
				return false;
			}
			
			//dumpTemplateToLog(t);
			
			// store it (by name) in a map for future reference
			
			templates.insert({ name, t });
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
				std::map<std::string, Template> * templates;
			};
			
			auto fetchTemplate = [](const char * name, void * user_data, Template & out_template) -> bool
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
					return loadTemplateFromFile(name, out_template);
				}
			};
			
			FetchContext context;
			context.templates = &templates;
			
			if (!applyTemplateOverlaysWithCallback(name, t, t, true, fetchTemplate, &context))
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
				if (!parseSceneObjectFromLines(typeDB, line_reader, out_scene, entities))
				{
					line_reader.pop_indent();
					return false;
				}
			}
			line_reader.pop_indent();
		}
		else
		{
			LOG_ERR("syntax error: %s", line);
			return false;
		}
	}
	
	return true;
}

bool parseSceneObjectFromLines(const TypeDB & typeDB, LineReader & line_reader, Scene & out_scene, std::map<std::string, Template> & templates)
{
	char * line; // fixme : use const char
	
	while ((line = (char*)line_reader.get_next_line(true)))
	{
		if (line[0] == '\t')
		{
			// only one level of identation may be added per line
			
			LOG_ERR("more than one level of identation added one line %d", line_reader.line_index);
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
			LOG_ERR("syntax error on line %d", line_reader.line_index);
			return false;
		}
	}
	
	return true;
}

bool parseSceneObjectStructureFromLines(const TypeDB & typeDB, LineReader & line_reader, Scene & out_scene, std::map<std::string, Template> & templates)
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
			// only one level of identation may be added per line
			
			LOG_ERR("more than one level of identation added one line %d", line_reader.line_index);
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
		node->id = out_scene.allocNodeId();
		node->parentId = node_stack.back()->id;
		node->displayName = name;
		node->components.add(new SceneNodeComponent());
		
		if (!instantiateComponentsFromTemplate(typeDB, t, node->components) || !node->initComponents())
		{
			LOG_ERR("failed to instantiate components from template", 0);
			
			node->freeComponents();
			
			delete node;
			node = nullptr;
			
			return false;
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
