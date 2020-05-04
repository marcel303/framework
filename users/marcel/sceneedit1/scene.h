#pragma once

#include "component.h"
#include "Mat4x4.h"
#include <map>
#include <string>
#include <vector>

class LineReader;
class LineWriter;
struct Scene;
struct SceneNode;
struct TypeDB;

struct SceneNode
{
	int id = -1;
	int parentId = -1; // note : not serialized, but inferred when loading a scene
	
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
	~Scene();
	
	int allocNodeId();
	
	void freeAllNodesAndComponents();
	
	void createRootNode();
	SceneNode & getRootNode();
	const SceneNode & getRootNode() const;
	SceneNode & getNode(const int nodeId);
	const SceneNode & getNode(const int nodeId) const;
	
	bool saveToLines(const TypeDB & typeDB, LineWriter & line_writer, const int indent);
	bool saveNodeHierarchyToLines(const int rootNodeId, LineWriter & line_writer, const int indent) const;
};
