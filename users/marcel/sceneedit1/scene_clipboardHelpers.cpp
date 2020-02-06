#include "component.h"
#include "componentType.h"
#include "helpers.h"
#include "lineReader.h"
#include "lineWriter.h"
#include "Log.h"
#include "scene.h"
#include "scene_clipboardHelpers.h"
#include "TextIO.h"

bool node_to_clipboard_text(const SceneNode & node, std::string & text)
{
	LineWriter line_writer;
	
	line_writer.append(":node\n");
	
	for (ComponentBase * component = node.components.head; component != nullptr; component = component->next_in_set)
	{
		auto * component_type = findComponentType(component->typeIndex());
		
		Assert(component_type != nullptr);
		if (component_type == nullptr)
			return false;
		
		line_writer.append_format("%s\n", component_type->typeName);
		
		if (object_tolines_recursive(g_typeDB, component_type, component, line_writer, 1) == false)
			continue;
	}
	
	text = line_writer.to_string();
	
	return true;
}

bool node_to_clipboard_text_recursive(const Scene & scene, const SceneNode & rootNode, std::string & text)
{
	// gather all nodes we will serialize later on
	
	std::vector<SceneNode> all_nodes;
	SceneNode sceneRootNode;
	sceneRootNode.parentId = -1;
	sceneRootNode.id = rootNode.parentId;
	sceneRootNode.childNodeIds.push_back(rootNode.id);
	all_nodes.push_back(sceneRootNode);
	
	std::vector<const SceneNode*> node_stack;
	node_stack.push_back(&sceneRootNode);
	
	while (node_stack.empty() == false)
	{
		auto * node = node_stack.back();
		node_stack.pop_back();
		
		all_nodes.push_back(*node);
		
		for (auto childNodeId : node->childNodeIds)
		{
			auto & childNode = scene.getNode(childNodeId);
			node_stack.push_back(&childNode);
		}
	}
	
	// allocate local node ids
	
	std::map<int, int> global_to_local_id;
	
	int nextLocalNodeId = 0;
	for (auto & node : all_nodes)
		global_to_local_id[node.id] = nextLocalNodeId++;
	
	// translate global node ids to local node ids
	
	auto global_to_local = [&](const int nodeId)
	{
		auto i = global_to_local_id.find(nodeId);
		assert(i != global_to_local_id.end());
		return i->second;
	};
	
	for (auto & node : all_nodes)
	{
		node.id = global_to_local(node.id);
		if (node.parentId != -1)
			node.parentId = global_to_local(node.parentId);
		for (auto & childNodeId : node.childNodeIds)
			childNodeId = global_to_local(childNodeId);
	}
	
	// create a temporary scene using the nodes we gathered
	
	Scene tempScene;
	
	for (auto & node : all_nodes)
		tempScene.nodes[node.id] = &node;
	
	tempScene.rootNodeId = global_to_local(sceneRootNode.id);
	
	// save scene to lines
	
	LineWriter line_writer;
	
	line_writer.append(":node-hierarchy\n");
	
	tempScene.saveToLines(g_typeDB, line_writer, 0);
	tempScene.nodes.clear();
	
	text = line_writer.to_string();
	
	return true;
}

bool node_from_clipboard_text(const char * text, SceneNode & node)
{
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (TextIO::loadText(text, lines, lineEndings) == false)
		return false;
	
	LineReader line_reader(lines, 0, 0);
	
	const char * id = line_reader.get_next_line(true);

	if (id == nullptr ||
		strcmp(id, ":node") != 0)
	{
		return false;
	}
	
	for (;;)
	{
		const char * component_type_name = line_reader.get_next_line(true);
		
		if (component_type_name[0] == '\t')
		{
			// only one level of identation may be added per line
			
			LOG_ERR("more than one level of identation added on line %d", line_reader.get_current_line_index());
			return false;
		}
		
		if (component_type_name == nullptr)
			break;
		
		auto * component_type = findComponentType(component_type_name);
		
		Assert(component_type != nullptr);
		if (component_type == nullptr)
		{
			node.freeComponents();
			return false;
		}
		
		auto * component = component_type->componentMgr->createComponent(nullptr);
		
		line_reader.push_indent();
		{
			if (object_fromlines_recursive(g_typeDB, component_type, component, line_reader) == false)
			{
				node.freeComponents();
				return false;
			}
		}
		line_reader.pop_indent();
		
		node.components.add(component);
	}
	
	return true;
}
