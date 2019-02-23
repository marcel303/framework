#include "framework.h"
#include "helpers.h"
#include "Log.h"
#include "scene.h"
#include "template.h"
#include "TextIO.h"
#include <map>
#include <string>
#include <vector>

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

static bool parseSceneObjectFromLines(std::vector<std::string> & lines, Scene & out_scene, std::map<std::string, Template> & templates);
static bool parseSceneObjectStructureFromLines(std::vector<std::string> & lines, Scene & out_scene, std::map<std::string, Template> & templates);

static bool parseSceneFromLines(std::vector<std::string> & lines)
{
	int current_level = -1;
	
	std::map<std::string, Template> templates;
	
	std::map<std::string, Template> entities;
	
	for (size_t i = 0; i < lines.size(); )
	{
		std::string & line = lines[i];
		
		// check for empty lines and skip them
		
		if (isEmptyLineOrComment(line.c_str()))
		{
			i++;
			continue;
		}
		
		const int new_level = calculateIndentationLevel(line.c_str());
		
		if (new_level > current_level + 1)
		{
			// only one level of identation may be added per line
			
			LOG_ERR("syntax error", 0);
			return false;
		}
		
		current_level = new_level;
		
		char * text = (char*)line.c_str() + current_level;
		
		if (current_level == 0)
		{
			const char * word;
	
			if (!eat_word_v2(text, word))
			{
				LOG_ERR("failed to eat word", 0);
				return false;
			}
			
			if (match_text(word, "template"))
			{
				const char * name;
				
				if (!eat_word_v2(text, name))
				{
					LOG_ERR("missing template name", 0);
					return false;
				}
				
				++i;
				
				// extract template lines, parse it, and store it (by name) in a map
				
				std::vector<std::string> template_lines;
				
				extractLinesGivenIndentationLevel(lines, i, current_level + 1, template_lines, true);
				
				// parse the template
				
				Template t;
				
			// todo : recursively parse using overlays
				if (!parseTemplateFromLines(template_lines, t))
				{
					LOG_ERR("failed to parse template", 0);
					return false;
				}
				
				dumpTemplateToLog(t);
				
				// store it (by name) in a map for future reference
				
				templates.insert({ name, t });
			}
			else if (match_text(word, "entity"))
			{
				const char * name;
				
				if (!eat_word_v2(text, name))
				{
					LOG_ERR("missing entity name", 0);
					return false;
				}
				
				++i;
				
				// todo : extract template lines and parse it
				
				std::vector<std::string> entity_lines;
				
				extractLinesGivenIndentationLevel(lines, i, current_level + 1, entity_lines, true);
				
				// parse the template
				
				Template t;
				
			// todo : recursively parse using overlays
				if (!parseTemplateFromLines(entity_lines, t))
				{
					LOG_ERR("failed to parse template (entity)", 0);
					return false;
				}
				
				dumpTemplateToLog(t);
				
				// todo : check if an entity with the same name exists already
				
				entities.insert({ name, t });
			}
			else if (match_text(word, "scene"))
			{
				++i;
				
				// todo : extract scene hieraerchy and parse it
				
				std::vector<std::string> scene_lines;
				
				extractLinesGivenIndentationLevel(lines, i, current_level + 1, scene_lines, true);
				
				LOG_DBG("%d lines", (int)scene_lines.size());
				
				Scene scene;
				
				if (!parseSceneObjectFromLines(scene_lines, scene, entities))
					return false;
			}
			else
			{
				LOG_ERR("syntax error: %s", line.c_str());
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	
	return true;
}

static bool parseSceneObjectFromLines(std::vector<std::string> & lines, Scene & out_scene, std::map<std::string, Template> & templates)
{
	int current_level = -1;
	
	for (size_t i = 0; i < lines.size(); )
	{
		char * line = (char*)lines[i].c_str();
		
		if (isEmptyLineOrComment(line))
		{
			++i;
			continue;
		}
		
		//
		
		const int new_level = calculateIndentationLevel(line);
		
		if (new_level > current_level + 1)
		{
			// only one level of identation may be added per line
			
			LOG_ERR("syntax error", 0);
			return false;
		}
		
		current_level = new_level;
		
		//
		
		if (current_level == 0)
		{
			const char * word;
			
			if (!eat_word_v2(line, word))
			{
				LOG_ERR("failed to eat word", 0);
				return false;
			}
			
			if (match_text(word, "nodes"))
			{
				++i;
				
				std::vector<std::string> nodes_lines;
		
				extractLinesGivenIndentationLevel(lines, i, current_level + 1, nodes_lines, true);
				
				if (!parseSceneObjectStructureFromLines(nodes_lines, out_scene, templates))
					return false;
			}
			else
			{
				LOG_ERR("syntax error", 0);
				return false;
			}
		}
		else
		{
			LOG_ERR("syntax error", 0);
			return false;
		}
	}
	
	return true;
}

static bool parseSceneObjectStructureFromLines(std::vector<std::string> & lines, Scene & out_scene, std::map<std::string, Template> & templates)
{
	int current_level = -1;
	
	std::vector<SceneNode*> node_stack;
	
	node_stack.push_back(&out_scene.getRootNode());
	
	for (size_t i = 0; i < lines.size(); )
	{
		char * line = (char*)lines[i].c_str();
		
		if (isEmptyLineOrComment(line))
		{
			++i;
			continue;
		}
		
		//
		
		const int new_level = calculateIndentationLevel(line);
		
		if (new_level > current_level + 1)
		{
			// only one level of identation may be added per line
			
			LOG_ERR("syntax error", 0);
			return false;
		}
		
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
		
		++i;
		
		auto template_itr = templates.find(name);
		
		if (template_itr == templates.end())
		{
			LOG_ERR("entity does not exist: %s", name);
			return false;
		}
		
		auto & t = template_itr->second;
		
	// todo : look up node by name. nodes should already be created when parsing entities
	// todo : make sure nodes are added to the scene only once. raise an error otherwise
		SceneNode * node = new SceneNode();
		node->id = out_scene.allocNodeId();
		node->parentId = node_stack.back()->id;
		
		if (!instantiateComponentsFromTemplate(t, node->components))
		{
			LOG_ERR("failed to instantiate components from template", 0);
			
			delete node;
			node = nullptr;
			
			return false;
		}
		
		out_scene.nodes.insert({ node->id, node });
		
		node_stack.push_back(node);
	}
	
	return true;
}

bool test_scenefiles()
{
	if (!framework.init(640, 480))
		return false;

	registerComponentTypes();

	// todo : load scene description text file

	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (!TextIO::load("textfiles/scene-v1.txt", lines, lineEndings))
	{
		LOG_ERR("failed to load text file", 0);
		return false;
	}

	if (!parseSceneFromLines(lines))
	{
		LOG_ERR("failed to parse scene from lines", 0);
		return false;
	}

	exit(0);

	return true;
}
