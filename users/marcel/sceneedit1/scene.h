#pragma once

#include "component.h"
#include "Mat4x4.h"
#include <map>
#include <string>
#include <vector>

struct ComponentJson;
struct Scene;
struct SceneNode;

struct SceneNode
{
	int id = -1;
	int parentId = -1;
	std::string displayName;
	
	std::vector<int> childNodeIds;
	
	ComponentSet components;
	
	bool initComponents();
	void freeComponents();
};

struct Scene
{
	std::map<int, SceneNode*> nodes;
	
	int nextNodeId = 0;
	
	int rootNodeId = -1;
	
	Scene();
	
	int allocNodeId();
	
	void createRootNode();
	SceneNode & getRootNode();
	const SceneNode & getRootNode() const;
	
#if ENABLE_COMPONENT_JSON
	bool save(ComponentJson & j);
	bool saveToFile(const char * filename);
	
	bool load(const ComponentJson & j);
	bool loadFromFile(const char * filename);
#endif
};
