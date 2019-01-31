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
	
	Mat4x4 objectToWorld = Mat4x4(true);
	
	ComponentSet components;
	
	void freeComponents();
};

struct Scene
{
	std::map<int, SceneNode*> nodes;
	
	int nextNodeId = 0;
	
	int rootNodeId = -1;
	
	Scene();
	
	int allocNodeId();
	
	SceneNode & getRootNode();
	const SceneNode & getRootNode() const;
	
	bool save(ComponentJson & j);
	bool load(const ComponentJson & j);
};
