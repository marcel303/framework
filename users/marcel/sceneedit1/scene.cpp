#include "components/sceneNodeComponent.h"
#include "componentType.h"
#include "Debugging.h"
#include "lineWriter.h"
#include "Log.h"
#include "scene.h"
#include "TextIO.h"

#include "helpers.h" // g_componentTypes

extern SceneNodeComponentMgr s_sceneNodeComponentMgr;

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

void Scene::freeNode(const int nodeId)
{
	Assert(nodes.count(nodeId) != 0);
	
	auto node_itr = nodes.find(nodeId);
	auto *& node = node_itr->second;
	
	node->freeComponents();
	
	delete node;
	node = nullptr;
	
	nodes.erase(node_itr);
}

void Scene::createRootNode()
{
	Assert(rootNodeId == -1);
	
	SceneNode * rootNode = new SceneNode();
	rootNode->id = allocNodeId();
	
	auto * sceneNodeComponent = s_sceneNodeComponentMgr.createComponent(rootNode->components.id);
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

SceneNode & Scene::getNode(const int nodeId)
{
	auto i = nodes.find(nodeId);
	Assert(i != nodes.end());
	
	return *i->second;
}

const SceneNode & Scene::getNode(const int nodeId) const
{
	auto i = nodes.find(nodeId);
	Assert(i != nodes.end());
	
	return *i->second;
}

static bool write_node_children_traverse(const Scene & scene, const int nodeId, LineWriter & line_writer, const int indent)
{
	auto & node = scene.getNode(nodeId);
	
	bool result = true;
	
	for (auto child_node_id : node.childNodeIds)
	{
		char id[32];
		sprintf(id, "%d", child_node_id); // todo : use a faster conversion function
	
		line_writer.append_indented_line(indent, id);
		
		result &= write_node_children_traverse(scene, child_node_id, line_writer, indent + 1);
	}
	
	return result;
}

bool Scene::saveToLines(const TypeDB & typeDB, LineWriter & line_writer, const int in_indent) const
{
	bool result = true;
	
	int indent = in_indent;
	
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
		
			result &= saveNodeHierarchyToLines(rootNodeId, line_writer, indent);
		}
		indent--;
	}
	indent--;
	
	Assert(indent == 0);

	return result;
}

bool Scene::saveNodeHierarchyToLines(const int rootNodeId, LineWriter & line_writer, const int indent) const
{
	if (rootNodeId == -1)
		return true;
	else
	{
		return write_node_children_traverse(
			*this,
			rootNodeId,
			line_writer,
			indent);
	}
}
