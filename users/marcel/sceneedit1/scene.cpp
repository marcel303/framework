#include "componentType.h"
#include "Debugging.h"
#include "lineWriter.h"
#include "Log.h"
#include "scene.h"
#include "sceneNodeComponent.h"
#include "TextIO.h"

#include "helpers.h" // g_componentTypes

bool SceneNode::initComponents()
{
	bool result = true;
	
	for (auto * component = components.head; component != nullptr; component = component->next_in_set)
	{
		if (component->init() == false)
		{
			auto * componentType = findComponentType(component->typeIndex());
			
			LOG_ERR("failed to initialize component '%s' of type %s",
				component->id,
				componentType == nullptr
					? "(unknown)"
					: componentType->typeName);
			
			result = false;
		}
	}
	
	return result;
}

void SceneNode::freeComponents()
{
	freeComponentsInComponentSet(components);
}

#if ENABLE_COMPONENT_JSON

#include "componentJson.h"

struct SceneNodeFromJson
{
	// this struct is just a silly little trick to make deserialization from json work. apparently from_json with target type 'SceneNode *&' is not allowed, so we cannot allocate objects and assign the pointer to the result. we use a struct with inside a pointer and later move the resultant objects into a vector again ..

	SceneNode * node = nullptr;
};

static void to_json(nlohmann::json & j, const SceneNode * node_ptr)
{
	auto & node = *node_ptr;

	j = nlohmann::json
	{
		{ "id", node.id },
		{ "displayName", node.displayName },
		{ "children", node.childNodeIds }
	};

	int component_index = 0;

	for (auto * component = node.components.head; component != nullptr; component = component->next_in_set)
	{
		// todo : save components
		
		auto * componentType = findComponentType(component->typeIndex());
		
		Assert(componentType != nullptr);
		if (componentType != nullptr)
		{
			if (strcmp(componentType->typeName, "SceneNodeComponent") == 0)
				continue;
			
			auto & components_json = j["components"];
			
			auto & component_json = components_json[component_index++];
			
			component_json["typeName"] = componentType->typeName;
			
			if (component->id[0] != 0)
			{
				component_json["id"] = component->id;
			}
			
			ComponentJson component_json_wrapped(component_json);
			
			if (member_tojson_recursive(g_typeDB, componentType, component, component_json_wrapped) == false)
			{
				LOG_ERR("failed to write component to json", 0);
				// todo : throw an exception ?
			}
		}
	}
}

void from_json(const nlohmann::json & j, SceneNodeFromJson & node_from_json)
{
	auto * node = new SceneNode();
	node->id = j.value("id", -1);
	node->displayName = j.value("displayName", "");
	node->childNodeIds = j.value("children", std::vector<int>());
	
	node->components.add(new SceneNodeComponent());
	
	node_from_json.node = node;
	
	auto components_json_itr = j.find("components");

	if (components_json_itr != j.end())
	{
		auto & components_json = *components_json_itr;
		
		for (auto & component_json : components_json)
		{
			const std::string typeName = component_json.value("typeName", "");
			const std::string id = component_json.value("id", "");
			
			if (typeName == "SceneNodeComponent")
				continue;
			
			if (typeName.empty())
			{
				LOG_WRN("empty type name", 0);
				continue;
			}
			
			auto * componentType = findComponentType(typeName.c_str());
			
			AssertMsg(componentType != nullptr, "could not find component type for type name '%s'", typeName.c_str());
			if (componentType != nullptr)
			{
				auto * component = componentType->componentMgr->createComponent(id.c_str());
				
				if (member_fromjson_recursive(g_typeDB, componentType, component, component_json) == false)
				{
					LOG_ERR("failed to read member from json", 0);
					// todo : throw an exception ?
				}
				
				node->components.add(component);
			}
		}
	}
}

#endif

Scene::Scene()
{
}

int Scene::allocNodeId()
{
	return nextNodeId++;
}

void Scene::createRootNode()
{
	Assert(rootNodeId == -1);
	
	SceneNode * rootNode = new SceneNode();
	rootNode->id = allocNodeId();
	rootNode->displayName = "root";
	
	rootNode->components.add(new SceneNodeComponent());
	
	if (rootNode->initComponents() == false)
	{
		rootNode->freeComponents();
		
		delete rootNode;
		rootNode = nullptr;
	}
	else
	{
		nodes[rootNode->id] = rootNode;
		
		rootNodeId = rootNode->id;
	}
}

SceneNode & Scene::getRootNode()
{
	auto i = nodes.find(rootNodeId);
	Assert(i != nodes.end());
	
	return *i->second;
}

const SceneNode & Scene::getRootNode() const
{
	auto i = nodes.find(rootNodeId);
	Assert(i != nodes.end());
	
	return *i->second;
}

#if ENABLE_COMPONENT_JSON

bool Scene::save(ComponentJson & jj)
{
	auto & j = jj.j;
	
	j["nextNodeId"] = nextNodeId;
	j["rootNodeId"] = rootNodeId;
	
	auto & nodes_json = j["nodes"];
	
	int node_index = 0;
	
	for (auto & node_itr : nodes)
	{
		auto & node = node_itr.second;
		auto & node_json = nodes_json[node_index++];
		
		node_json = node;
	}
	
	return true;
}

bool Scene::saveToFile(const char * filename)
{
	nlohmann::json j;
	ComponentJson jj(j);
	
	if (!save(jj))
		return false;
	
	auto text = j.dump(4);

	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	if (!TextIO::loadText(text.c_str(), lines, lineEndings))
		return false;
	
	return TextIO::save(filename, lines, TextIO::kLineEndings_Unix);
}

bool Scene::load(const ComponentJson & jj)
{
	bool result = true;
	
	auto & j = jj.j;
	
	nextNodeId = j.value("nextNodeId", -1);
	rootNodeId = j.value("rootNodeId", -1);
	
	auto nodes_from_json = j.value("nodes", std::vector<SceneNodeFromJson>());
	
	if (result)
	{
		// initialize components
		
		for (auto & node_from_json : nodes_from_json)
		{
			auto * node = node_from_json.node;
			
			if (node->initComponents() == false)
			{
				LOG_ERR("failed to initialize one or more components in node component set", 0);
				result &= false;
			}
		}
	}
	
	if (result)
	{
		// add nodes and check for duplicate ids
		
		for (auto & node_from_json : nodes_from_json)
		{
			auto * node = node_from_json.node;
			
			Assert(nodes.count(node->id) == 0);
			if (nodes.count(node->id) == 0)
			{
				nodes[node->id] = node;
			}
			else
			{
				LOG_ERR("duplicate node id: %d", node->id);
				result &= false;
			}
		}
		
		// set up scene node hierarchy
		
		for (auto & node_itr : nodes)
		{
			auto * node = node_itr.second;
			
			for (auto & childNodeId : node->childNodeIds)
			{
				auto childNode_itr = nodes.find(childNodeId);
				
				Assert(childNode_itr != nodes.end());
				if (childNode_itr != nodes.end())
				{
					auto * childNode = childNode_itr->second;
					
					childNode->parentId = node->id;
				}
				else
				{
					LOG_ERR("invalid child node id in scene node hierarchy. node=%d, child_node=%d", node->id, childNodeId);
					result &= false;
				}
			}
		}
	}
	
	// clean up if something failed
	
	if (result == false)
	{
		nodes.clear();
		
		for (auto & node_from_json : nodes_from_json)
		{
			auto * node = node_from_json.node;
			
			node->freeComponents();
			
			delete node;
			node = nullptr;
		}
	}
	
	return result;
}

bool Scene::loadFromFile(const char * filename)
{
	bool result = true;

	char * text;
	size_t textSize;

	if (result == true)
	{
		result &= TextIO::loadFileContents(filename, text, textSize);
	}

	nlohmann::json j;

	if (result == true)
	{
		try
		{
			j = nlohmann::json::parse(text);
		}
		catch (std::exception & e)
		{
			LOG_ERR("failed to parse JSON: %s", e.what());
			result &= false;
		}
		
		delete [] text;
		text = nullptr;
	}
	
	if (result == true)
	{
		ComponentJson jj(j);
		
		result &= load(jj);
	}
	
	return result;
}

#endif

bool Scene::saveToLines(const TypeDB & typeDB, LineWriter & line_writer)
{
	bool result = true;
	
	int indent = 0;
	
	for (auto & node_itr : nodes)
	{
		auto & node = node_itr.second;
		
		char node_definition[128];
		sprintf(node_definition, "entity %d", node->id);
		line_writer.AppendIndentedLine(indent, node_definition);
		
		indent++;
		{
			for (auto * component = node->components.head; component != nullptr; component = component->next_in_set)
			{
				auto * componentType = findComponentType(component->typeIndex());
				
				if (componentType == nullptr) // todo : error
					continue;
				
				if (strcmp(componentType->typeName, "SceneNodeComponent") == 0)
					continue;
				
				// todo : make short version of component type name
				
				if (component->id[0] != 0)
				{
					char component_definition[128];
					sprintf(component_definition, "%s %s", componentType->typeName, component->id);
					line_writer.AppendIndentedLine(indent, component_definition);
				}
				else
				{
					line_writer.AppendIndentedLine(indent, componentType->typeName);
				}
				
				indent++;
				{
					result &= object_tolines_recursive(typeDB, componentType, component, line_writer, indent);
				}
				indent--;
			}
		}
		indent--;
	}
	
	//
	
	line_writer.AppendIndentedLine(indent, "scene");
	
	indent++;
	{
		line_writer.AppendIndentedLine(indent, "nodes");
		
		indent++;
		{
			// todo : write node hierarchy
		}
		indent--;
	}
	indent--;
	
	Assert(indent == 0);

	return result;
}
