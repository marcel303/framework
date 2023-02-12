#include "componentType.h"
#include "componentTypeDB.h"
#include "Debugging.h"
#include "helpers.h" // freeComponentsInComponentSet
#include "lineWriter.h"
#include "Log.h"
#include "reflection-textio.h"
#include "scene.h"
#include "sceneNodeComponent.h"
#include "StringEx.h"
#include "TextIO.h"

#include <algorithm>
#include <set>

// -- scene node

bool SceneNode::initComponents()
{
	bool result = true;
	
	for (auto * component = components.head; component != nullptr; component = component->next_in_set)
	{
		if (component->init() == false)
		{
			auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
			
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

// -- scene

Scene::~Scene()
{
	Assert(nodes.empty());
}

int Scene::allocNodeId()
{
	if (nodeIdAllocator != nullptr)
		return nodeIdAllocator->allocNodeId();
	else
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

bool Scene::initComponents()
{
	bool result = true;

	for (auto node_itr : nodes)
	{
		auto * node = node_itr.second;
		
		result &= node->initComponents();
	}

	return result;
}

void Scene::createRootNode()
{
	Assert(rootNodeId == -1);
	
	SceneNode * rootNode = new SceneNode();
	rootNode->id = allocNodeId();
	rootNode->name = "root";
	
	auto * sceneNodeComponent = g_sceneNodeComponentMgr.createComponent(rootNode->components.id);
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

void Scene::removeNodeSubTree(const int rootNodeId, std::vector<int> * out_removedNodeIds)
{
	auto & node = getNode(rootNodeId);
	
	// recursively remove child nodes
	
	while (!node.childNodeIds.empty())
	{
		const int childNodeId = node.childNodeIds.front();
		removeNodeSubTree(childNodeId, out_removedNodeIds);
	}
	Assert(node.childNodeIds.empty());
	
	// remove our reference from our parent
	
	if (node.parentId != -1)
	{
		auto & parentNode = getNode(node.parentId);
		auto childNodeId_itr = std::find(parentNode.childNodeIds.begin(), parentNode.childNodeIds.end(), node.id);
		Assert(childNodeId_itr != parentNode.childNodeIds.end());
		parentNode.childNodeIds.erase(childNodeId_itr);
	}
	
	// free the node
	
	freeNode(rootNodeId);
	
	if (out_removedNodeIds != nullptr)
		out_removedNodeIds->push_back(rootNodeId);
}

void Scene::collectNodeSubTreeNodeIds(const int rootNodeId, std::vector<int> & out_nodeIds) const
{
	out_nodeIds.push_back(rootNodeId);
	
	auto & node = getNode(rootNodeId);
	
	for (auto childNodeId : node.childNodeIds)
		collectNodeSubTreeNodeIds(childNodeId, out_nodeIds);
}

void Scene::assignAutoGeneratedNodeNames() const
{
	// determine the set of node names for save/load already in-use. we will need this set later when assigning auto-generated names
	
	std::set<std::string> nodeNames;
	
	for (auto & node_itr : nodes)
	{
		auto * node = node_itr.second;
		
		if (!node->name.empty())
			nodeNames.insert(node->name);
	}
	
	// assign auto-generated names for nodes without a name
	
	int autoGeneratedNameIndex = 1;
	
	for (auto & node_itr : nodes)
	{
		auto * node = node_itr.second;
		
		if (node->name.empty())
		{
			// generate a new name until we found a unique one
			
			for (;;)
			{
				char name[64];
				sprintf_s(name, sizeof(name), "%d", autoGeneratedNameIndex++);
				
				if (nodeNames.count(name) == 0)
				{
					// found a unique name. assign it and end trying
					node->name = name;
					break;
				}
			}
		}
	}
}

bool Scene::validate() const
{
	bool result = true;
	
	std::set<std::string> usedNodeNames;
	
	for (auto & node_itr : nodes)
	{
		auto * node = node_itr.second;
		
		// validate the name isn't a duplicate
		
		if (!node->name.empty())
		{
			if (usedNodeNames.count(node->name) != 0)
			{
				LOG_ERR("the scene node name '%s' is used multiple times by node %d", node->name.c_str(), node->id);
				result &= false;
			}
			
			usedNodeNames.insert(node->name);
		}
		
		// validate child node references
		
		for (auto childNodeId : node->childNodeIds)
		{
			auto i = nodes.find(childNodeId);
			
			if (i == nodes.end())
			{
				LOG_ERR("node %d has a child node reference to non-existing node %d", childNodeId);
				result &= false;
			}
		}
	}
	
	return result;
}
