#include "componentJson.h"
#include "componentType.h"
#include "Debugging.h"
#include "Log.h"
#include "scene.h"
#include "TextIO.h"

#include "helpers.h" // g_componentTypes

struct SceneNodeFromJson
{
	// this struct is just a silly little trick to make deserialization from json work. apparently from_json with target type 'SceneNode *&' is not allowed, so we cannot allocate objects and assign the pointer to the result. we use a struct with inside a pointer and later move the resultant objects into a vector again ..

	SceneNode * node = nullptr;
};

bool SceneNode::initComponents()
{
	bool result = true;
	
	for (auto * component = components.head; component != nullptr; component = component->next_in_set)
		result &= component->init();
	
	return result;
}

void SceneNode::freeComponents()
{
	ComponentBase * next;
	
	for (auto * component = components.head; component != nullptr; component = next)
	{
		// the component will be removed and next_in_set will become invalid, so we need to fetch it now
		
		next = component->next_in_set;
		
	// todo : give error when failure to find component manager. this would imply a memory leak
		for (auto * componentType : g_componentTypes)
		{
			auto * componentMgr = componentType->componentMgr;
			
			if (component->typeIndex() != componentMgr->typeIndex())
				continue;
		
			componentMgr->removeComponent(component);
		}
	}
	
	components.head = nullptr;
}

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
			auto & components_json = j["components"];
			
			auto & component_json = components_json[component_index++];
			
			component_json["typeName"] = componentType->typeName;
			
			if (component->id[0] != 0)
			{
				component_json["id"] = component->id;
			}
			
			for (auto & property : componentType->properties)
			{
				ComponentJson property_json(component_json[property->name]);
				
				property->to_json(component, property_json);
			}
		}
	}
}

void from_json(const nlohmann::json & j, SceneNodeFromJson & node_from_json)
{
	node_from_json.node = new SceneNode();

	auto & node = *node_from_json.node;
	node.id = j.value("id", -1);
	node.displayName = j.value("displayName", "");
	node.childNodeIds = j.value("children", std::vector<int>());

	auto components_json_itr = j.find("components");

	if (components_json_itr != j.end())
	{
		auto & components_json = *components_json_itr;
		
		for (auto & component_json : components_json)
		{
			const std::string typeName = component_json.value("typeName", "");
			const std::string id = component_json.value("id", "");
			
			if (typeName.empty())
			{
				LOG_WRN("empty type name", 0);
				continue;
			}
			
			auto * componentType = findComponentType(typeName.c_str());
			
			Assert(componentType != nullptr);
			if (componentType != nullptr)
			{
				auto * component = componentType->componentMgr->createComponent(id.c_str());
				
				for (auto & property : componentType->properties)
				{
					if (component_json.count(property->name) != 0)
						property->from_json(component, component_json);
				}
				
				if (component->init())
					node.components.add(component);
				else
					componentType->componentMgr->removeComponent(component);
			}
		}
	}
}

Scene::Scene()
{
	SceneNode & rootNode = *new SceneNode();
	rootNode.id = allocNodeId();
	rootNode.displayName = "root";
	
	nodes[rootNode.id] = &rootNode;
	
	rootNodeId = rootNode.id;
}

int Scene::allocNodeId()
{
	return nextNodeId++;
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
	auto & j = jj.j;
	
	nextNodeId = j.value("nextNodeId", -1);
	rootNodeId = j.value("rootNodeId", -1);
	
	auto nodes_from_json = j.value("nodes", std::vector<SceneNodeFromJson>());
	
	for (auto & node_from_json : nodes_from_json)
	{
		auto * node = node_from_json.node;
		
		nodes[node->id] = node;
	}
	
	for (auto & node_itr : nodes)
	{
		auto & node = *node_itr.second;
		
		for (auto & childNodeId : node.childNodeIds)
		{
			auto childNode_itr = nodes.find(childNodeId);
			
			Assert(childNode_itr != nodes.end());
			if (childNode_itr != nodes.end())
			{
				auto & childNode = *childNode_itr->second;
				
				childNode.parentId = node.id;
			}
		}
	}
	
	return true;
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
