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
			
			LOG_ERR("failed to initialize component of type %s",
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

Scene::Scene()
{
}

Scene::~Scene()
{
	Assert(nodes.empty());
}

int Scene::allocNodeId()
{
	return nextNodeId++;
}

void Scene::freeAllNodesAndComponents()
{
	for (auto node_itr : nodes)
	{
		auto * node = node_itr.second;
		
		node->freeComponents();
		
		delete node;
		node = nullptr;
	}
	
	nodes.clear();
	
	rootNodeId = -1;
}

void Scene::createRootNode()
{
	Assert(rootNodeId == -1);
	
	SceneNode * rootNode = new SceneNode();
	rootNode->id = allocNodeId();
	
	auto * sceneNodeComponent = new SceneNodeComponent();
	sceneNodeComponent->name = "root";
	rootNode->components.add(sceneNodeComponent);
	
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

static void write_node_children_traverse(const Scene & scene, const int nodeId, LineWriter & line_writer, const int indent)
{
	auto node_itr = scene.nodes.find(nodeId);
	Assert(node_itr != scene.nodes.end());
	if (node_itr == scene.nodes.end())
		return;
	
	auto * node = node_itr->second;
	
	for (auto child_node_id : node->childNodeIds)
	{
		char id[32];
		sprintf(id, "%d", child_node_id); // todo : use a faster conversion function
	
		line_writer.append_indented_line(indent, id);
		
		write_node_children_traverse(scene, child_node_id, line_writer, indent + 1);
	}
}

bool Scene::saveToLines(const TypeDB & typeDB, LineWriter & line_writer)
{
	bool result = true;
	
	int indent = 0;
	
	for (auto & node_itr : nodes)
	{
		auto & node = node_itr.second;
		
		char node_definition[128];
		sprintf(node_definition, "entity %d", node->id);
		line_writer.append_indented_line(indent, node_definition);
		
		indent++;
		{
			for (auto * component = node->components.head; component != nullptr; component = component->next_in_set)
			{
				auto * componentType = findComponentType(component->typeIndex());
				
				if (componentType == nullptr) // todo : error
					continue;
				
				// todo : make short version of component type name
				line_writer.append_indented_line(indent, componentType->typeName);
				
				indent++;
				{
					result &= object_tolines_recursive(typeDB, componentType, component, line_writer, indent);
				}
				indent--;
			}
		}
		indent--;
		
		line_writer.append('\n');
	}
	
	//
	
	line_writer.append_indented_line(indent, "scene");
	
	indent++;
	{
		line_writer.append_indented_line(indent, "nodes");
		
		indent++;
		{
			// write node hierarchy
			
			write_node_children_traverse(*this, rootNodeId, line_writer, indent);
		}
		indent--;
	}
	indent--;
	
	Assert(indent == 0);

	return result;
}
