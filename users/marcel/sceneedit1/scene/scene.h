#pragma once

#include "component.h" // ComponentSet
#include <map> // nodes
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
	std::string name;  // unique name, used during save/load only
	
	int id = -1;       // id used for run-time lookups. note : not serialized, but allocated when a node is created
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
	
	Scene * nodeIdAllocator = nullptr;
	
	~Scene();
	
	int allocNodeId();
	
	void freeAllNodesAndComponents();
	void freeNode(const int nodeId);
	
	bool initComponents();
	
	void createRootNode();
	
	      SceneNode & getRootNode();
	const SceneNode & getRootNode() const;
	      SceneNode & getNode(const int nodeId);
	const SceneNode & getNode(const int nodeId) const;
	
	void removeNodeSubTree(const int rootNodeId, std::vector<int> * out_removedNodeIds = nullptr);
	
	void collectNodeSubTreeNodeIds(const int rootNodeId, std::vector<int> & out_nodeIds) const;
	
	void assignAutoGeneratedNodeNames() const;
	
	bool validate() const;
};
