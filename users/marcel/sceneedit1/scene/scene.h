#pragma once

#include "component.h"
#include <map>
#include <string>
#include <vector>

// forward declarations

class LineReader;
class LineWriter;
struct Scene;
struct SceneNode;
struct TypeDB;

//

struct SceneNode
{
	std::string name; // unique name, used during save/load only
	
	int id = -1; // id used for run-time lookups. note : not serialized, but allocated when a node is created
	int parentId = -1; // parent id used for run-time lookups. note : not serialized, but inferred when loading a scene
	
	std::vector<int> childNodeIds;
	
	ComponentSet components;
	
	bool initComponents();
	void freeComponents();
};

//

struct Scene
{
	std::map<int, SceneNode*> nodes;
	
	int nextNodeId = 0;
	
	int rootNodeId = -1;
	
	~Scene();
	
	int allocNodeId();
	
	void freeAllNodesAndComponents();
	void freeNode(const int nodeId);
	
	void createRootNode();
	
	      SceneNode & getRootNode();
	const SceneNode & getRootNode() const;
	      SceneNode & getNode(const int nodeId);
	const SceneNode & getNode(const int nodeId) const;
	
	bool saveToLines(const TypeDB & typeDB, LineWriter & line_writer, const int indent) const;
	bool saveNodeHierarchyToLines(const int rootNodeId, LineWriter & line_writer, const int indent) const;
	
	bool validate() const;
};
