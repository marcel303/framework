// ecs-sceneEditor
#include "editor/componentPropertyUi.h"
#include "editor/parameterComponentUi.h"
#include "editor/raycast.h"
#include "editor/scene_clipboardHelpers.h"
#include "editor/sceneEditor.h"
#include "editor/transformGizmos.h"

// ecs-scene
#define DEFINE_COMPONENT_TYPES
#include "components/transformComponent.h"
#include "helpers.h" // findComponentType
#include "scene.h"
#include "sceneIo.h"
#include "sceneNodeComponent.h"
#include "templateIo.h"

// ecs-component
#include "componentType.h"

// ecs-parameter
#include "parameterUi.h"

// imgui-framework
#include "imgui-framework.h"

// framework
#include "framework.h"
#include "framework-camera.h"

// libreflection-textio
#include "lineReader.h"
#include "lineWriter.h"

// libgg
#include "Path.h"
#include "Quat.h"
#include "StringEx.h"
#include "TextIO.h"

// libsdl
#include <SDL2/SDL.h>

// std
#include <set>

//

// todo : remove. need a may generic way to ask for a node's bounding box

#include "components/gltfComponent.h"
#include "components/modelComponent.h"
#include "components/parameterComponent.h"
extern GltfComponentMgr s_gltfComponentMgr;
extern ModelComponentMgr s_modelComponentMgr;
extern ParameterComponentMgr s_parameterComponentMgr;

//

extern SceneNodeComponentMgr s_sceneNodeComponentMgr;
extern TransformComponentMgr s_transformComponentMgr;

//

inline bool modShift()
{
	return
		keyboard.isDown(SDLK_LSHIFT) ||
		keyboard.isDown(SDLK_RSHIFT);
}

inline bool keyCommand()
{
	return
		keyboard.isDown(SDLK_LGUI) ||
		keyboard.isDown(SDLK_RGUI);
}

//

void SceneEditor::init(TypeDB * in_typeDB)
{
	Assert(in_typeDB != nullptr);
	
	typeDB = in_typeDB;
	
	scene.createRootNode();
	
	camera.mode = Camera::kMode_Orbit;
	camera.orbit.distance = -8.f;
	camera.ortho.side = Camera::kOrthoSide_Top;
	camera.ortho.scale = 16.f;
	camera.ortho.speed = 1.f;
	camera.firstPerson.position = Vec3(0, 1, -2);
	camera.firstPerson.height = 0.f;

	guiContext.init();
	
	undoReset();
}

void SceneEditor::shut()
{
	scene.freeAllNodesAndComponents();
	
	guiContext.shut();
}

SceneNode * SceneEditor::raycast(Vec3Arg rayOrigin, Vec3Arg rayDirection, const std::set<int> & nodesToIgnore) const
{
	SceneNode * bestNode = nullptr;
	float bestDistance = std::numeric_limits<float>::max();
	
	for (const auto & nodeItr : scene.nodes)
	{
		auto & node = *nodeItr.second;
		
		if (nodesToIgnore.count(node.id) != 0)
			continue;
		
		const auto * sceneNodeComp = node.components.find<SceneNodeComponent>();
		const auto * modelComponent = node.components.find<ModelComponent>();
		
		Assert(sceneNodeComp != nullptr);
		if (sceneNodeComp != nullptr && modelComponent != nullptr && modelComponent->hasModelAabb)
		{
			const auto & objectToWorld = sceneNodeComp->objectToWorld;
			const auto worldToObject = objectToWorld.CalcInv();
			
			const Vec3 rayOrigin_object = worldToObject.Mul4(rayOrigin);
			const Vec3 rayDirection_object = worldToObject.Mul3(rayDirection);
			
			float distance;
			if (intersectBoundingBox3d(
				&modelComponent->aabbMin[0],
				&modelComponent->aabbMax[0],
				rayOrigin_object[0],
				rayOrigin_object[1],
				rayOrigin_object[2],
				1.f / rayDirection_object[0],
				1.f / rayDirection_object[1],
				1.f / rayDirection_object[2],
				distance))
			{
				if (distance < bestDistance)
				{
					bestNode = &node;
					bestDistance = distance;
				}
			}
		}
	}
	
	return bestNode;
}

/**
 * Begin a deferred update to the scene structure. Note we need to defer updates, due to
 * the fact that the scene editor traverses the scene structure at the same time as making
 * updates to it. Making immediate updates would invalidate and/or complicate scene traversal.
 */
void SceneEditor::deferredBegin()
{
	Assert(deferred.numActivations || deferred.isFlushed());
	
	deferred.numActivations++;
}

void SceneEditor::deferredEnd()
{
	Assert(deferred.isProcessing == false);
	Assert(deferred.numActivations > 0);
	
	deferred.numActivations--;
	
	if (deferred.numActivations == 0 && deferred.isFlushed() == false)
	{
		undoCaptureBegin();
		{
			deferred.isProcessing = true;
			
			addNodesToAdd();
			
			removeNodesToRemove();
			
			selectNodesToSelect(false);
			
			deferred.isProcessing = false;
		}
		undoCaptureEnd();
		
		Assert(deferred.isFlushed());
	}
}

void SceneEditor::removeNodeSubTree(const int nodeId)
{
	AssertMsg(deferred.isProcessing, "this method may only be called by deferredEnd!", 0);
	
	std::vector<int> removedNodeIds;
	scene.removeNodeSubTree(nodeId, &removedNodeIds);
	
	for (auto nodeId : removedNodeIds)
	{
		// update node references
		
		if (nodeId == scene.rootNodeId)
			scene.rootNodeId = -1;
		
		selectedNodes.erase(nodeId);
		
		deferred.nodesToRemove.erase(nodeId);
		deferred.nodesToSelect.erase(nodeId);
	}
}

void SceneEditor::addNodesToAdd()
{
	AssertMsg(deferred.isProcessing, "this method may only be called by deferredEnd!", 0);
	
	for (SceneNode * node : deferred.nodesToAdd)
	{
		Assert(scene.nodes.find(node->id) == scene.nodes.end());
		
		// insert the node into the scene
		scene.nodes[node->id] = node;
		
		// insert the node into the child list of its parent, if it isn't inserted yet
		auto & parentNode = scene.getNode(node->parentId);
		const bool isInsertedIntoParent =
			std::find(
				parentNode.childNodeIds.begin(),
				parentNode.childNodeIds.end(),
				node->id) != parentNode.childNodeIds.end();
		if (isInsertedIntoParent == false)
			parentNode.childNodeIds.push_back(node->id);
	}
	
	deferred.nodesToAdd.clear();
}

void SceneEditor::removeNodesToRemove()
{
	AssertMsg(deferred.isProcessing, "this method may only be called by deferredEnd!", 0);
	
	while (!deferred.nodesToRemove.empty())
	{
		auto nodeToRemoveItr = deferred.nodesToRemove.begin();
		auto nodeId = *nodeToRemoveItr;
		
		removeNodeSubTree(nodeId);
	}
}

void SceneEditor::selectNodesToSelect(const bool append)
{
	AssertMsg(deferred.isProcessing, "this method may only be called by deferredEnd!", 0);
	
	if (!deferred.nodesToSelect.empty())
	{
		if (append == false)
		{
			selectedNodes.clear();
		}
		
		for (auto nodeId : deferred.nodesToSelect)
		{
			selectedNodes.insert(nodeId);
			nodeToGiveFocus = nodeId;
			
			auto & node = scene.getNode(nodeId);
			markNodeOpenUntilRoot(node.parentId);
		}
		
		deferred.nodesToSelect.clear();
	}
}

bool SceneEditor::undoCapture(std::string & text) const
{
	LineWriter line_writer;
	
	if (writeSceneToLines(*typeDB, scene, line_writer, 0) == false)
	{
		logError("failed to save scene to lines");
		return false;
	}
	else
	{
		text = line_writer.to_string();
		return true;
	}
}

void SceneEditor::undoCaptureBegin()
{
#if defined(DEBUG)
	// for debugging purposes, we save the current version,
	// and compare it later when we begin capturing a new undo
	// state. these SHOULD be equal, unless some action didn't
	// get recorded into undo
	
	std::string text;
	
	if (undoCapture(text))
	{
	// fixme : entity ids aren't reserved between load/save actions. we should have 1. unique entity names (for scene hierarchy save/load) and 2. unique entity ids (for components and fast lookups)
		//Assert(text == undo.currentVersion);
	}
#endif
}

void SceneEditor::undoCaptureEnd()
{
	std::string text;
	
	if (undoCapture(text))
	{
		undo.versions.resize(undo.currentVersionIndex + 1);
		undo.versions.push_back(text);
		undo.currentVersionIndex = undo.versions.size() - 1;
		
	#if defined(DEBUG)
		undo.currentVersion = text;
	#endif
	}
}

void SceneEditor::undoReset()
{
	undo.versions.clear();
	undo.currentVersion.clear();
	undo.currentVersionIndex = -1;
	
	undoCapture(undo.currentVersion);
	undo.versions.push_back(undo.currentVersion);
	undo.currentVersionIndex = undo.versions.size() - 1;
}

void SceneEditor::editNode(const int nodeId)
{
	// todo : make a separate function to edit a data structure (recursively)
	
	ImGui::PushID(nodeId);
	{
		auto & node = scene.getNode(nodeId);
		
		// collect the list of components for this node. optionally filtered by component type name
		
		const bool do_filter = nodeUi.componentTypeNameFilter[0] != 0;
		
		const int kMaxComponents = 256;
		ComponentBase * sorted_components[kMaxComponents];
		int num_components = 0;
		
		for (auto * component = node.components.head; component != nullptr && num_components < kMaxComponents; component = component->next_in_set)
		{
			if (component->isType<SceneNodeComponent>())
				continue;
			
			bool passes_filter = true;
			
			if (do_filter)
			{
				const auto * componentType = findComponentType(component->typeIndex());
				
				passes_filter =
					componentType != nullptr &&
					strcasestr(componentType->typeName, nodeUi.componentTypeNameFilter) != nullptr;
			}
			
			if (passes_filter)
			{
				sorted_components[num_components++] = component;
			}
		}
		
		Assert(num_components < kMaxComponents);
		
		// sort components by type name
		
		std::sort(sorted_components, sorted_components + num_components,
			[](const ComponentBase * first, const ComponentBase * second) -> bool
				{
					const auto * first_type = findComponentType(first->typeIndex());
					const auto * second_type = findComponentType(second->typeIndex());
					
					if (first_type == nullptr || second_type == nullptr)
						return first_type > second_type;
					else
						return strcmp(first_type->typeName, second_type->typeName) < 0;
				});
		
		// show component editors
		
		for (int i = 0; i < num_components; ++i)
		{
			ComponentBase * component = sorted_components[i];
			
			ImGui::PushID(component);
			{
				// note : we group component properties here, so the node context menu can be opened by
				// right clicking anywhere inside this group
				
				ImGui::BeginGroup();
				{
					const auto * componentType = findComponentType(component->typeIndex());
					
					Assert(componentType != nullptr);
					if (componentType != nullptr)
					{
						ImGui::LabelText("", "%s", componentType->typeName);
						ImGui::Indent();
						
						bool isSet = true;
						void * changedMemberObject = nullptr;
						
						if (doReflection_StructuredType(*typeDB, *componentType, component, isSet, nullptr, &changedMemberObject))
						{
							// signal the component one of its properties has changed
							component->propertyChanged(changedMemberObject);
						}
						
						ImGui::Unindent();
					}
				}
				ImGui::EndGroup();
			
			#if 1
				// see if we should open the node context menu
				
				NodeContextMenuResult result = kNodeContextMenuResult_None;
			
				//const bool canOpen = !ImGui::IsPopupOpen(nullptr) || ImGui::IsPopupOpen("NodeMenu");
				const bool canOpen = true; // fixme : this override any popup context menu that was opened inside the component property editors
				
				if (canOpen && ImGui::BeginPopupContextItem("NodeMenu"))
				{
					result = doNodeContextMenu(node, component);

					ImGui::EndPopup();
				}
			
				if (result == kNodeContextMenuResult_ComponentShouldBeRemoved)
				{
					undoCaptureBegin();
					{
						freeComponentInComponentSet(node.components, component);
					}
					undoCaptureEnd();
				}
			#endif
			}
			ImGui::PopID();
		}
		
		if (num_components == 0)
		{
			// no components exist or passed the filter, but we still need some area for the user to click to add nodes and components
			
			ImGui::Text("(no components");
			
			if (ImGui::BeginPopupContextItem("NodeMenu"))
			{
				doNodeContextMenu(node, nullptr);

				ImGui::EndPopup();
			}
		}
	}
	ImGui::PopID();
}

void SceneEditor::updateClipboardInfo()
{
	clipboardInfo = ClipboardInfo();
	
	// fetch lines from clipboard
	
	clipboardInfo.lines = linesFromClipboard();
	
	// scan through the clipboard contents to see what's stored inside
	
	LineReader line_reader(clipboardInfo.lines, 0, 0);
	line_reader.disable_jump_check();
	
	for (;;)
	{
		const char * id = line_reader.get_next_line(true);
		
		if (id == nullptr)
			break;
		
		// check if we have a component inside the clipboard, and remember its type if so
		
		if (strcmp(id, "component") == 0)
		{
			line_reader.push_indent();
			{
				const char * typeName = line_reader.get_next_line(true);
				
				if (typeName != nullptr && typeName[0] != '\t')
				{
					char full_name[1024];
					if (expandComponentTypeName(typeName, full_name, sizeof(full_name)))
					{
						clipboardInfo.hasComponent = true;
						clipboardInfo.componentTypeName = full_name;
						clipboardInfo.component_lineIndex = line_reader.get_current_line_index();
						clipboardInfo.component_lineIndent = line_reader.get_current_indentation_level() + 1;
					}
				}
			}
			line_reader.pop_indent();
		}
	}
}

bool SceneEditor::pasteNodeFromText(const int parentId, LineReader & line_reader)
{
	SceneNode * childNode = new SceneNode();
	childNode->id = scene.allocNodeId();
	childNode->parentId = parentId;
	
	if (pasteSceneNodeFromLines(*typeDB, line_reader, *childNode) == false)
	{
		delete childNode;
		childNode = nullptr;
		
		return false;
	}
	else
	{
		if (childNode->components.contains<SceneNodeComponent>() == false)
		{
			auto * sceneNodeComponent = s_sceneNodeComponentMgr.createComponent(childNode->components.id);
			childNode->components.add(sceneNodeComponent);
		}
		
		auto * sceneNodeComponent = childNode->components.find<SceneNodeComponent>();
		sceneNodeComponent->name = String::FormatC("Node %d", childNode->id);
		
		if (childNode->initComponents() == false)
		{
			childNode->freeComponents();
			
			delete childNode;
			childNode = nullptr;
			
			return false;
		}
		else
		{
			deferredBegin();
			{
				deferred.nodesToAdd.push_back(childNode);
				deferred.nodesToSelect.insert(childNode->id);
			}
			deferredEnd();
			
			return true;
		}
	}
}

bool SceneEditor::pasteNodeTreeFromText(const int parentId, LineReader & line_reader)
{
	Scene tempScene;
	tempScene.nodeIdAllocator = &scene;
	
	// create root node, assign its parent id, and make sure it has a transform component
	tempScene.createRootNode();
	auto & rootNode = tempScene.getRootNode();
	rootNode.parentId = parentId;
	
	if (!pasteSceneNodeTreeFromLines(*typeDB, line_reader, tempScene))
	{
		tempScene.freeAllNodesAndComponents();
		return false;
	}
	
	bool init_ok = true;

	for (auto node_itr : tempScene.nodes)
	{
		auto & node = *node_itr.second;
		
		node.name.clear(); // make sure it gets assigned a new unique auto-generated name on save
		
		if (node.components.contains<SceneNodeComponent>() == false)
		{
			auto * sceneNodeComponent = s_sceneNodeComponentMgr.createComponent(node.components.id);
			node.components.add(sceneNodeComponent);
		}
		
		init_ok &= node.initComponents();
	}

	if (init_ok == false)
	{
		tempScene.freeAllNodesAndComponents();
		return false;
	}
	else
	{
		deferredBegin();
		{
		// todo : let deferredEnd traverse child nodes and add them ?
			for (auto node_itr : tempScene.nodes)
			{
				auto * node = node_itr.second;
				
				if (node->id == rootNode.id)
				{
					node->freeComponents();
					continue;
				}
				
				if (node->parentId == rootNode.id)
				{
					node->parentId = parentId;
					deferred.nodesToSelect.insert(node->id);
				}
				
				deferred.nodesToAdd.push_back(node);
			}
		}
		deferredEnd();
		
		tempScene.nodes.clear();
		
		return true;
	}
}

SceneEditor::NodeStructureContextMenuResult SceneEditor::doNodeStructureContextMenu(SceneNode & node)
{
	//logDebug("context window for %d", node.id);
	
	NodeStructureContextMenuResult result = kNodeStructureContextMenuResult_None;

	auto * sceneNodeComponent = node.components.find<SceneNodeComponent>();
	
	char name[256];
	strcpy_s(name, sizeof(name), sceneNodeComponent->name.c_str());
	if (ImGui::InputText("Name", name, sizeof(name)))
	{
		sceneNodeComponent->name = name;
	}
	
// todo : add the ability to attach a scene
	if (ImGui::MenuItem("Attach from template"))
	{
		deferredBegin();
		{
			//addNodeFromTemplate_v2(Vec3(), AngleAxis(), node.id);
			addNodesFromScene_v1(node.id);
		}
		deferredEnd();
	}
	
	if (ImGui::MenuItem("Copy"))
	{
		result = kNodeStructureContextMenuResult_NodeCopy;
		performAction_copy(false);
	}
	
	if (ImGui::MenuItem("Copy tree"))
	{
		result = kNodeStructureContextMenuResult_NodeCopy;
		performAction_copy(true);
	}
	
	if (ImGui::MenuItem("Paste as child", nullptr, false, clipboardInfo.lines.empty() == false))
	{
		result = kNodeStructureContextMenuResult_NodePaste;
		performAction_paste(node.id);
	}
	
	if (ImGui::MenuItem("Paste as sibling", nullptr, false, clipboardInfo.lines.empty() == false && node.parentId != -1))
	{
		result = kNodeStructureContextMenuResult_NodePaste;
		performAction_paste(node.parentId);
	}
	
	if (ImGui::MenuItem("Remove", nullptr, false, node.parentId != -1))
	{
		result = kNodeStructureContextMenuResult_NodeQueuedForRemove;
		
		deferred.nodesToRemove.insert(node.id);
	}

	if (ImGui::MenuItem("Add child node"))
	{
		result = kNodeStructureContextMenuResult_NodeAdded;
		
		SceneNode * childNode = new SceneNode();
		childNode->id = scene.allocNodeId();
		childNode->parentId = node.id;
		
		auto * sceneNodeComponent = s_sceneNodeComponentMgr.createComponent(childNode->components.id);
		sceneNodeComponent->name = String::FormatC("Node %d", childNode->id);
		childNode->components.add(sceneNodeComponent);
		
		auto * transformComponent = s_transformComponentMgr.createComponent(childNode->components.id);
		childNode->components.add(transformComponent);
		
		if (childNode->initComponents() == false)
		{
			childNode->freeComponents();
			
			delete childNode;
			childNode = nullptr;
		}
		else
		{
			deferredBegin();
			{
				deferred.nodesToAdd.push_back(childNode);
				deferred.nodesToSelect.insert(childNode->id);
			}
			deferredEnd();
		}
	}
	
	return result;
}

SceneEditor::NodeContextMenuResult SceneEditor::doNodeContextMenu(SceneNode & node, ComponentBase * selectedComponent)
{
	//logDebug("context window for %d", node.id);
	
	NodeContextMenuResult result = kNodeContextMenuResult_None;
	
	if (selectedComponent != nullptr)
	{
		if (ImGui::MenuItem("Copy"))
		{
			bool result = true;
			
			LineWriter line_writer;
			
			int indent = 0;
			
			line_writer.append_indented_line(indent, "component");
			
			indent++;
			{
				result &= writeComponentToLines(*typeDB, *selectedComponent, line_writer, indent);
			}
			indent--;
			
			if (result)
			{
				const std::string text = line_writer.to_string();
				SDL_SetClipboardText(text.c_str());
			}
		}
	}
	
	// check if we have a component inside the clipboard. if so, add options for pasting the component either as a new component or just its values
	
	bool hasComponentOfType = false;
	
	if (clipboardInfo.hasComponent)
	{
		// see if we already have a component of this type. we use this information later to show the correct menu option ('paste as new component' vs 'paste component values')
		
		for (auto * component = node.components.head; component != nullptr; component = component->next_in_set)
		{
			auto * componentType = findComponentType(component->typeIndex());
			if (componentType != nullptr && strcmp(componentType->typeName, clipboardInfo.componentTypeName.c_str()) == 0)
				hasComponentOfType = true;
		}
	}
	
	if (ImGui::MenuItem("Paste as new component", nullptr, false, clipboardInfo.hasComponent && !hasComponentOfType))
	{
		LineReader line_reader(clipboardInfo.lines, clipboardInfo.component_lineIndex, clipboardInfo.component_lineIndent);
		
		auto * componentType = findComponentType(clipboardInfo.componentTypeName.c_str());
		
		if (componentType != nullptr)
		{
			auto * component = componentType->componentMgr->createComponent(node.components.id);
			
			undoCaptureBegin();
			{
				if (parseComponentFromLines(*typeDB, line_reader, *component) == false || component->init() == false)
					componentType->componentMgr->destroyComponent(node.components.id);
				else
					node.components.add(component);
			}
			undoCaptureEnd();
		}
	}
	
	if (ImGui::MenuItem("Paste component values", nullptr, false, clipboardInfo.hasComponent && hasComponentOfType))
	{
		LineReader line_reader(clipboardInfo.lines, clipboardInfo.component_lineIndex, clipboardInfo.component_lineIndent);
		
		for (auto * component = node.components.head; component != nullptr; component = component->next_in_set)
		{
			auto * componentType = findComponentType(component->typeIndex());
			
			if (componentType != nullptr && strcmp(componentType->typeName, clipboardInfo.componentTypeName.c_str()) == 0)
			{
				undoCaptureBegin();
				{
					parseComponentFromLines(*typeDB, line_reader, *component);
					
					for (auto * member = componentType->members_head; member != nullptr; member = member->next)
					{
						if (!member->isVector)
						{
							Member_Scalar * member_scalar = static_cast<Member_Scalar*>(member);
							void * member_ptr = member_scalar->scalar_access(component);
							component->propertyChanged(member_ptr);
						}
					}
				}
				undoCaptureEnd();
				
				break;
			}
		}
	}
	
	if (selectedComponent != nullptr && !selectedComponent->isType<SceneNodeComponent>())
	{
		if (ImGui::MenuItem("Remove component"))
		{
			result = kNodeContextMenuResult_ComponentShouldBeRemoved;
		}
	}

	if (ImGui::BeginMenu("Add component.."))
	{
		for (auto * componentType : g_componentTypes)
		{
			// check if the node already has a component of this type
			
			bool hasComponent = false;
			
			for (auto * component = node.components.head; component != nullptr; component = component->next_in_set)
				if (component->typeIndex() == componentType->componentMgr->typeIndex())
					hasComponent = true;
			
			// show the option to add a component of this type if the node doesn't have it yet
			
			if (hasComponent == false)
			{
				char text[256];
				sprintf_s(text, sizeof(text), "Add %s", componentType->typeName);
				
				if (ImGui::MenuItem(text))
				{
					result = kNodeContextMenuResult_ComponentAdded;
					
					undoCaptureBegin();
					{
						auto * component = componentType->componentMgr->createComponent(node.components.id);
						
						if (component->init())
							node.components.add(component);
						else
							componentType->componentMgr->destroyComponent(node.components.id);
					}
					undoCaptureEnd();
				}
			}
		}
		
		ImGui::EndMenu();
	}
	
	return result;
}

void SceneEditor::editNodeStructure_traverse(const int nodeId)
{
	const bool do_filter = nodeUi.nodeDisplayNameFilter[0] != 0;
	
	auto & node = scene.getNode(nodeId);
	
	//
	
	const bool isSelected = selectedNodes.count(node.id) != 0;
	const bool isLeaf = node.childNodeIds.empty();
	const bool isRoot = node.parentId == -1;
	
	ImGui::PushID(&node);
	{
		// update scrolling
		
		if (nodeToGiveFocus == nodeId)
		{
			nodeToGiveFocus = -1;
			ImGui::SetScrollHereY();
		}
		
		// update visibility
		
		if (nodeUi.visibleNodes.count(node.id) != 0 ||
			nodeUi.nodesToOpen_active.count(node.id) != 0)
		{
			ImGui::SetNextTreeNodeOpen(true);
		}
		
		// show the tree node
		
		const char * name = "(noname)";
		
		auto * sceneNodeComponent = node.components.find<SceneNodeComponent>();
		Assert(sceneNodeComponent != nullptr);
		if (sceneNodeComponent != nullptr)
			name = sceneNodeComponent->name.c_str();
		
		const bool isOpen = ImGui::TreeNodeEx(&node,
			(ImGuiTreeNodeFlags_OpenOnArrow * 1) |
			(ImGuiTreeNodeFlags_Selected * isSelected) |
			(ImGuiTreeNodeFlags_Leaf * isLeaf) |
			(ImGuiTreeNodeFlags_DefaultOpen * isRoot) |
			(ImGuiTreeNodeFlags_FramePadding * 0 |
			(ImGuiTreeNodeFlags_NavLeftJumpsBackHere * 1)), "%s", name);
		
		const bool isClicked = ImGui::IsItemClicked();
		
		if (isClicked)
		{
			if (!ImGui::GetIO().KeyShift)
				selectedNodes.clear();
			selectedNodes.insert(node.id);
		}
	
		if (ImGui::BeginPopupContextItem("NodeStructureMenu"))
		{
			doNodeStructureContextMenu(node);

			ImGui::EndPopup();
		}

		if (isOpen)
		{
			for (auto childNodeId : node.childNodeIds)
			{
				if (do_filter && nodeUi.visibleNodes.count(childNodeId) == 0)
				{
					continue;
				}

				editNodeStructure_traverse(childNodeId);
			}
		
			ImGui::TreePop();
		}
	}
	ImGui::PopID();
}

void SceneEditor::updateNodeVisibility()
{
	// determine the set of visible nodes, given any filters applied to the list
	// note that when a child node is visible, all its elders are visible as well
	
	nodeUi.visibleNodes.clear();
	
	if (nodeUi.nodeDisplayNameFilter[0] != 0 ||
		nodeUi.componentTypeNameFilter[0] != 0)
	{
		auto markNodeVisibleUntilRoot = [&](const int nodeId)
		{
			nodeUi.visibleNodes.insert(nodeId);

			auto & node = scene.getNode(nodeId);
			
			int parentId = node.parentId;

			while (parentId != -1)
			{
				if (nodeUi.visibleNodes.count(parentId) != 0)
					break;
				
				nodeUi.visibleNodes.insert(parentId);
				
				auto & parentNode = scene.getNode(parentId);
				parentId = parentNode.parentId;
			}
		};
		
		// selected nodes are always visible
		
		for (auto nodeId : selectedNodes)
		{
			markNodeVisibleUntilRoot(nodeId);
		}
		
		// check each node, to see if it passes the filter(s)
		
		for (auto & node_itr : scene.nodes)
		{
			const auto nodeId = node_itr.first;
			const auto * node = node_itr.second;
			
			if (nodeUi.visibleNodes.count(nodeId) != 0)
				continue;
			
			bool is_visible = true;
			
			if (nodeUi.nodeDisplayNameFilter[0] != 0)
			{
				const auto * sceneNodeComponent = node->components.find<SceneNodeComponent>();
				
				is_visible &=
					sceneNodeComponent != nullptr &&
					strcasestr(sceneNodeComponent->name.c_str(), nodeUi.nodeDisplayNameFilter);
			}
			
			if (nodeUi.componentTypeNameFilter[0] != 0)
			{
				bool passes_component_filter = false;
				
				for (auto * component = node->components.head; component != nullptr; component = component->next_in_set)
				{
					auto * componentType = findComponentType(component->typeIndex());
					Assert(componentType != nullptr);
					if (componentType != nullptr)
						passes_component_filter |= strcasestr(componentType->typeName, nodeUi.componentTypeNameFilter) != nullptr;
				}
				
				is_visible &= passes_component_filter;
			}
			
			if (is_visible)
			{
				markNodeVisibleUntilRoot(nodeId);
			}
		}
	}
}

void SceneEditor::markNodeOpenUntilRoot(const int in_nodeId)
{
	int nodeId = in_nodeId;

	while (nodeId != -1)
	{
		if (nodeUi.nodesToOpen_deferred.count(nodeId) != 0)
			break;
		
		nodeUi.nodesToOpen_deferred.insert(nodeId);
		
		auto & node = scene.getNode(nodeId);
		nodeId = node.parentId;
	}
}

// todo : this is a test method. remove from scene editor and move elsewhere
void SceneEditor::addNodeFromTemplate_v1(Vec3Arg position, const AngleAxis & angleAxis, const int parentId)
{
	SceneNode * node = new SceneNode();
	node->id = scene.allocNodeId();
	node->parentId = parentId;
	
	auto * sceneNodeComponent = s_sceneNodeComponentMgr.createComponent(node->components.id);
	sceneNodeComponent->name = String::FormatC("Node %d", node->id);
	node->components.add(sceneNodeComponent);
	
	auto * modelComp = s_modelComponentMgr.createComponent(node->components.id);
	modelComp->filename = "model.txt";
	modelComp->scale = .01f;
	node->components.add(modelComp);
	
	auto * transformComp = s_transformComponentMgr.createComponent(node->components.id);
	transformComp->position = position;
	node->components.add(transformComp);
	
	if (node->initComponents() == false)
	{
		node->freeComponents();
		
		delete node;
		node = nullptr;
	}
	else
	{
		scene.nodes[node->id] = node;
		
		//
		

		auto & parentNode = scene.getNode(parentId);
		parentNode.childNodeIds.push_back(node->id);
	}
}

// todo : this is a test method. remove from scene editor and move elsewhere
#include "template.h"
#include "templateIo.h"
int SceneEditor::addNodeFromTemplate_v2(Vec3Arg position, const AngleAxis & angleAxis, const int parentId)
{
	Template t;
	
	if (!parseTemplateFromFileAndRecursivelyOverlayBaseTemplates(
		"textfiles/base-entity-v1-overlay.txt",
		true,
		true,
		t))
	{
		return -1;
	}
	
	//
	
	SceneNode * node = new SceneNode();
	node->id = scene.allocNodeId();
	node->parentId = parentId;
	
	bool init_ok = true;
	
	init_ok &= instantiateComponentsFromTemplate(*typeDB, t, node->components);
	
	if (node->components.contains<SceneNodeComponent>() == false)
	{
		auto * sceneNodeComponent = s_sceneNodeComponentMgr.createComponent(node->components.id);
		sceneNodeComponent->name = String::FormatC("Node %d", node->id);
		node->components.add(sceneNodeComponent);
	}
	
	init_ok &= node->initComponents();
	
	if (init_ok == false)
	{
		logError("failed to initialize node components");
		
		node->freeComponents();
		
		delete node;
		node = nullptr;
		
		return -1;
	}
	
	{
		auto * transformComp = node->components.find<TransformComponent>();
		
		if (transformComp != nullptr)
		{
			transformComp->position = position;
			transformComp->angleAxis = angleAxis;
		}
	}
	
	deferredBegin();
	{
		deferred.nodesToAdd.push_back(node);
		deferred.nodesToSelect.insert(node->id);
	}
	deferredEnd();
	
	return node->id;
}

// todo : this is a test method. remove from scene editor and move elsewhere
int SceneEditor::addNodesFromScene_v1(const int parentId)
{
	Scene tempScene;
	tempScene.nodeIdAllocator = &scene;
	
	// create root node, assign its parent id, and make sure it has a transform component
	tempScene.createRootNode();
	auto & rootNode = tempScene.getRootNode();
	rootNode.parentId = parentId;
	auto * transformComponent = s_transformComponentMgr.createComponent(rootNode.components.id);
	rootNode.components.add(transformComponent);
	
	if (!parseSceneFromFile(*typeDB, "textfiles/scene-v1.txt", tempScene))
	{
		tempScene.freeAllNodesAndComponents();
		return -1;
	}
	
	for (auto node_itr : tempScene.nodes)
		node_itr.second->name.clear(); // make sure it gets assigned a new unique auto-generated name on save
	
	bool init_ok = true;
	
	for (auto node_itr : tempScene.nodes)
		init_ok &= node_itr.second->initComponents();

	if (init_ok == false)
	{
		tempScene.freeAllNodesAndComponents();
		return -1;
	}
	else
	{
		deferredBegin();
		{
			for (auto node_itr : tempScene.nodes)
			{
				deferred.nodesToAdd.push_back(node_itr.second);
			}
			
			deferred.nodesToSelect.insert(tempScene.rootNodeId);
			
			tempScene.nodes.clear();
		}
		deferredEnd();
		
		return tempScene.rootNodeId;
	}
}

void SceneEditor::tickEditor(const float dt, bool & inputIsCaptured)
{
	updateClipboardInfo();
	
	int viewSx;
	int viewSy;
	framework.getCurrentViewportSize(viewSx, viewSy);
	
	guiContext.processBegin(dt, viewSx, viewSy, inputIsCaptured);
	{
		if (showUi)
		{
			ImGui::SetNextWindowPos(ImVec2(4, 20 + 4 + 4));
			ImGui::SetNextWindowSize(ImVec2(370, viewSy - 20 - 4 - 4));
			if (ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
			{
				// visibility options
				
				if (ImGui::CollapsingHeader("Visibility", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Indent();
					{
						ImGui::Checkbox("Draw grid", &visibility.drawGrid);
						ImGui::Checkbox("Draw ground plane", &visibility.drawGroundPlane);
						ImGui::Checkbox("Draw nodes", &visibility.drawNodes);
						ImGui::Checkbox("Draw node bounding boxes", &visibility.drawNodeBoundingBoxes);
					}
					ImGui::Unindent();
				}
				
				// preview/simulation options
				
				if (ImGui::CollapsingHeader("Preview", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::Indent();
					{
						ImGui::Checkbox("Tick", &preview.tickScene);
						ImGui::SameLine();
						ImGui::Checkbox("Draw", &preview.drawScene);
						ImGui::SameLine();
						ImGui::PushItemWidth(80.f);
						ImGui::PushID("Tick Mult");
						ImGui::SliderFloat("", &preview.tickMultiplier, 0.f, 100.f, "%.2f", 4.f);
						ImGui::PopID();
						ImGui::PopItemWidth();
						ImGui::SameLine();
						if (ImGui::Button("100%"))
						{
							preview.tickScene = true;
							preview.tickMultiplier = 1.f;
						}
						ImGui::SameLine();
						if (ImGui::Button("--"))
						{
							preview.tickScene = true;
							preview.tickMultiplier /= 1.25f;
						}
						ImGui::SameLine();
						if (ImGui::Button("++"))
						{
							preview.tickScene = true;
							preview.tickMultiplier *= 1.25f;
						}
					}
					ImGui::Unindent();
				}
				
				// node structure
				
				if (ImGui::CollapsingHeader("Scene structure", ImGuiTreeNodeFlags_DefaultOpen))
				{
					if (ImGui::InputText("Display name", nodeUi.nodeDisplayNameFilter, kMaxNodeDisplayNameFilter))
					{
						updateNodeVisibility();
					}
					
					if (ImGui::InputText("Component type", nodeUi.componentTypeNameFilter, kMaxComponentTypeNameFilter))
					{
						updateNodeVisibility();
					}
					
					updateNodeVisibility();
					
					ImGui::BeginChild("Scene structure", ImVec2(0, 140), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
					{
						deferredBegin();
						{
							nodeUi.nodesToOpen_active = nodeUi.nodesToOpen_deferred;
							nodeUi.nodesToOpen_deferred.clear();
							
							editNodeStructure_traverse(scene.rootNodeId);
						}
						deferredEnd();
					}
					ImGui::EndChild();
					
					if (ImGui::IsItemHovered())
					{
						if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)) ||
							ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
						{
							deferredBegin();
							for (auto nodeId : selectedNodes)
								deferred.nodesToRemove.insert(nodeId);
							deferredEnd();
						}
					}
				}
				
				// node editing
				
				if (ImGui::CollapsingHeader("Selected node(s)", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::BeginChild("Selected nodes", ImVec2(0, 300), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
					{
						ImGui::PushItemWidth(180.f);
						for (auto & selectedNodeId : selectedNodes)
						{
							editNode(selectedNodeId);
						}
						ImGui::PopItemWidth();
					}
					ImGui::EndChild();
					
					// one or more transforms may have been edited or invalidated due to a parent node being invalidated
					// ensure the transforms are up to date by recalculating them. this is needed for transform gizmos
					// to work
					s_transformComponentMgr.calculateTransforms(scene);
				}
			}
			ImGui::End();
		}
		
		// menu bar
		
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Save"))
					performAction_save();
				if (ImGui::MenuItem("Load"))
					performAction_load();
				
				ImGui::EndMenu();
			}
			
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", nullptr, false, undo.currentVersionIndex > 0))
					performAction_undo();
				if (ImGui::MenuItem("Redo", nullptr, false, undo.currentVersionIndex + 1 < undo.versions.size()))
					performAction_redo();
				ImGui::Separator();
				
				if (ImGui::MenuItem("Copy", nullptr, false, !selectedNodes.empty()))
					performAction_copy(false);
				if (ImGui::MenuItem("Copy tree", nullptr, false, !selectedNodes.empty()))
					performAction_copy(true);
				if (ImGui::MenuItem("Paste", nullptr, false, clipboardInfo.lines.empty() == false))
					performAction_paste(scene.rootNodeId);
				ImGui::Separator();
				
				if (ImGui::MenuItem("Duplicate", nullptr, false, !selectedNodes.empty()))
					performAction_duplicate();
				
				ImGui::EndMenu();
			}
			
		#if ENABLE_RENDERER
			if (ImGui::BeginMenu("Renderer"))
			{
				parameterUi::doParameterUi(renderer.parameterMgr, nullptr, false);
				
				ImGui::EndMenu();
			}
		#endif
			
			if (ImGui::BeginMenu("Parameters"))
			{
				ImGui::InputText("Component filter", parameterUi.component_filter, kMaxParameterFilter);
				ImGui::InputText("Parameter filter", parameterUi.parameter_filter, kMaxParameterFilter);
				ImGui::Checkbox("Show components with empty prefix", &parameterUi.showAnonymousComponents);
				
				doParameterUi(s_parameterComponentMgr, parameterUi.component_filter, parameterUi.parameter_filter, parameterUi.showAnonymousComponents);
				
				ImGui::EndMenu();
			}
		}
		ImGui::EndMainMenuBar();
	
		guiContext.updateMouseCursor();
	}
	guiContext.processEnd();
	
	// action: show/hide ui
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_TAB))
	{
		inputIsCaptured = true;
		showUi = !showUi;
	}
	
	// action: save
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_s) && keyCommand())
	{
		inputIsCaptured = true;
		performAction_save();
	}
	
	// action: load
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_l) && keyCommand())
	{
		inputIsCaptured = true;
		performAction_load();
	}
	
	// action: increase simulation speed
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_a) && keyCommand())
	{
		inputIsCaptured = true;
		preview.tickMultiplier *= 1.25f;
	}
	
	// action: decrease simulation speed
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_z) && keyCommand())
	{
		inputIsCaptured = true;
		preview.tickMultiplier /= 1.25f;
	}
	
	// action: copy selected node(s) to clipboard
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_c) && keyCommand())
	{
		inputIsCaptured = true;
		const bool deep = modShift();
		performAction_copy(deep);
	}
	
	// action: paste node(s) from clipboard
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_v) && keyCommand())
	{
		inputIsCaptured = true;
		performAction_paste(scene.rootNodeId);
	}
	
	// action: duplicate selected node(s)
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_d) && keyCommand())
	{
		inputIsCaptured = true;
		performAction_duplicate();
	}
	
	// -- mouse picking
	
	// 1. transform mouse coordinates into a world space direction vector

	int viewportSx;
	int viewportSy;
	framework.getCurrentViewportSize(viewportSx, viewportSy);

	Mat4x4 projectionMatrix;
	camera.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);
	
	Mat4x4 cameraToWorld;
	camera.calculateWorldMatrix(cameraToWorld);
	
	Vec3 cameraPosition;
	Vec3 mouseDirection_world;
	
	if (camera.mode == Camera::kMode_Ortho)
	{
		const Vec2 mousePosition_screen(
			mouse.x,
			mouse.y);
		const Vec3 mousePosition_clip(
			mousePosition_screen[0] / float(viewportSx) * 2.f - 1.f,
			mousePosition_screen[1] / float(viewportSy) * 2.f - 1.f,
			0.f);
		Vec3 mousePosition_camera = projectionMatrix.CalcInv().Mul4(mousePosition_clip);
		mousePosition_camera[1] = -mousePosition_camera[1];
		
		cameraPosition = cameraToWorld.Mul4(mousePosition_camera) - cameraToWorld.GetAxis(2) * 10.f;
		
		mouseDirection_world = cameraToWorld.GetAxis(2);
		mouseDirection_world = mouseDirection_world.CalcNormalized();
	}
	else
	{
		cameraPosition = cameraToWorld.GetTranslation();
		
		const Vec2 mousePosition_screen(
			mouse.x,
			mouse.y);
		const Vec2 mousePosition_clip(
			mousePosition_screen[0] / float(viewportSx) * 2.f - 1.f,
			mousePosition_screen[1] / float(viewportSy) * 2.f - 1.f);
		Vec2 mousePosition_camera = projectionMatrix.CalcInv().Mul4(mousePosition_clip);
		
		mouseDirection_world = cameraToWorld.Mul3(
			Vec3(
				+mousePosition_camera[0],
				-mousePosition_camera[1],
				1.f));
		mouseDirection_world = mouseDirection_world.CalcNormalized();
	}
	
#if ENABLE_TRANSFORM_GIZMOS
	// 2. transformation gizo interaction
	
	if (selectedNodes.size() != 1)
	{
		transformGizmo.hide();
	}
	else
	{
		auto & node = scene.getNode(*selectedNodes.begin());
		
		// determine the current global transform
		
		Mat4x4 globalTransform(true);
		
		auto * sceneNodeComponent = node.components.find<SceneNodeComponent>();
		Assert(sceneNodeComponent != nullptr);
		
		if (sceneNodeComponent != nullptr)
			globalTransform = sceneNodeComponent->objectToWorld;
		
		// let the gizmo do it's thing
		
		transformGizmo.show(globalTransform);
		
		if (transformGizmo.tick(cameraPosition, mouseDirection_world, inputIsCaptured))
		{
			// transform the global transform into local space
			
			Mat4x4 localTransform = transformGizmo.gizmoToWorld;
			
			if (node.parentId != -1)
			{
				auto & parentNode = scene.getNode(node.parentId);
				
				auto * sceneNodeComponent_parent = parentNode.components.find<SceneNodeComponent>();
				Assert(sceneNodeComponent_parent != nullptr);
			
				if (sceneNodeComponent_parent != nullptr)
					localTransform = sceneNodeComponent_parent->objectToWorld.CalcInv() * localTransform;
			}
			
			// assign the translation in local space to the transform component
			
			auto * transformComponent = node.components.find<TransformComponent>();
			
			if (transformComponent != nullptr)
			{
				transformComponent->position = localTransform.GetTranslation();
				
				for (int i = 0; i < 3; ++i)
				{
					//logDebug("size[%d] = %.2f", i, localTransform.GetAxis(i).CalcSize());
					localTransform.SetAxis(i, localTransform.GetAxis(i).CalcNormalized());
				}
				
				Quat q;
				q.fromMatrix(localTransform);
				
			#if ENABLE_QUAT_FIXUP
				// todo : not entirely sure if this is correct. need a proper look at the maths and logic here
				int max_axis = 0;
				if (fabsf(transformComponent->angleAxis.axis[1]) > fabsf(transformComponent->angleAxis.axis[max_axis]))
					max_axis = 1;
				if (fabsf(transformComponent->angleAxis.axis[2]) > fabsf(transformComponent->angleAxis.axis[max_axis]))
					max_axis = 2;
				const bool neg_before = transformComponent->angleAxis.axis[max_axis] < 0.f;
			#endif
			
				q.toAxisAngle(transformComponent->angleAxis.axis, transformComponent->angleAxis.angle);
				
			#if ENABLE_QUAT_FIXUP && false
				const bool neg_after = transformComponent->angleAxis.axis[max_axis] < 0.f;
				if (neg_before != neg_after)
				{
					transformComponent->angleAxis.axis = -transformComponent->angleAxis.axis;
					transformComponent->angleAxis.angle = -transformComponent->angleAxis.angle;
				}
			#endif
			
				transformComponent->angleAxis.angle = transformComponent->angleAxis.angle * 180.f / float(M_PI);
			}
		}
	}
#endif

	// 3. determine which node is underneath the mouse cursor
	
	const SceneNode * hoverNode =
		inputIsCaptured == false
		? raycast(cameraPosition, mouseDirection_world, selectedNodes)
		: nullptr;
	
	hoverNodeId = hoverNode ? hoverNode->id : -1;
	
	// 4. update mouse cursor
	
// todo : framework should mediate here. perhaps have a requested mouse cursor and update it each frame. or make it a member of the mouse, and add an explicit mouse.updateCursor() method

	if (inputIsCaptured == false)
	{
		static SDL_Cursor * cursorHand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		SDL_SetCursor(hoverNode == nullptr ? SDL_GetDefaultCursor() : cursorHand);
	}
	
	if (inputIsCaptured == false && mouse.wentDown(BUTTON_LEFT))
	{
		// action: add node from template. todo : this is a test action. remove! instead, have a template list or something and allow adding instances from there
		if (keyboard.isDown(SDLK_RSHIFT))
		{
			inputIsCaptured = true;
			
			// todo : make action dependent on editing state. in this case, node placement
		
			// find intersection point with the ground plane
		
			if (mouseDirection_world[1] != 0.f)
			{
				const float t = -cameraPosition[1] / mouseDirection_world[1];
				
				if (t >= 0.f)
				{
					const Vec3 groundPosition = cameraPosition + mouseDirection_world * t;
					
					if (keyboard.isDown(SDLK_c))
					{
						deferredBegin();
						{
							for (auto & parentNodeId : selectedNodes)
							{
								auto & parentNode = scene.getNode(parentNodeId);
								
								auto * sceneNodeComp = parentNode.components.find<SceneNodeComponent>();
								
								Assert(sceneNodeComp != nullptr);
								if (sceneNodeComp != nullptr)
								{
									const Vec3 groundPosition_parent = sceneNodeComp->objectToWorld.CalcInv().Mul4(groundPosition);
									
									addNodeFromTemplate_v2(groundPosition_parent, AngleAxis(), parentNodeId);
								}
							}
						}
						deferredEnd();
					}
					else
					{
						deferredBegin();
						{
							addNodeFromTemplate_v2(groundPosition, AngleAxis(), scene.rootNodeId);
						}
						deferredEnd();
					}
				}
			}
		}
		else
		{
			if (!modShift())
				selectedNodes.clear();
			
			if (hoverNode != nullptr)
			{
				selectedNodes.insert(hoverNode->id);
				markNodeOpenUntilRoot(hoverNode->id);
				nodeToGiveFocus = hoverNode->id;
			}
		}
	}
	
	// action: remove selected node(s)
	if (inputIsCaptured == false && (keyboard.wentDown(SDLK_BACKSPACE) || keyboard.wentDown(SDLK_DELETE)))
	{
		inputIsCaptured = true;
		
		deferredBegin();
		{
			for (auto nodeId : selectedNodes)
			{
				if (nodeId == scene.rootNodeId)
					continue;
				
				deferred.nodesToRemove.insert(nodeId);
			}
		}
		deferredEnd();
	}
	
	// update the camera
	
	if (camera.mode == Camera::kMode_FirstPerson)
		cameraIsActive = mouse.isDown(BUTTON_LEFT);
	else
		cameraIsActive = true;
	
	camera.tick(dt, inputIsCaptured, cameraIsActive == false);
	
	// perform debug checks
	
#if defined(DEBUG)
	validateNodeReferences();
	validateNodeStructure();
#endif
}

void SceneEditor::validateNodeReferences() const
{
	for (auto & selectedNodeId : selectedNodes)
		Assert(scene.nodes.count(selectedNodeId) != 0);
	Assert(hoverNodeId == -1 || scene.nodes.count(hoverNodeId) != 0);
	Assert(nodeToGiveFocus == -1 || scene.nodes.count(nodeToGiveFocus) != 0);
	Assert(deferred.isFlushed());
}

void SceneEditor::validateNodeStructure() const
{
	std::set<int> usedNodeIds;
	std::set<SceneNode*> usedNodes;
	std::set<std::string> usedNodeNames;
	for (auto & node_itr : scene.nodes)
	{
		Assert(node_itr.first == node_itr.second->id);
		Assert(usedNodeIds.count(node_itr.first) == 0);
		Assert(usedNodes.count(node_itr.second) == 0);
		Assert(usedNodeNames.count(node_itr.second->name) == 0);
		Assert(node_itr.second->parentId == -1 || scene.nodes.count(node_itr.second->parentId) != 0);
		if (node_itr.second->parentId != -1)
		{
			auto & parentNode = scene.getNode(node_itr.second->parentId);
			Assert(
				std::find(
					parentNode.childNodeIds.begin(),
					parentNode.childNodeIds.end(),
					node_itr.second->id) != parentNode.childNodeIds.end());
		}
		usedNodeIds.insert(node_itr.first);
		usedNodes.insert(node_itr.second);
		usedNodeNames.insert(node_itr.second->name);
	}
}

void SceneEditor::performAction_save()
{
#if ENABLE_SAVE_LOAD_TIMING
	auto t1 = SDL_GetTicks();
#endif
	
	LineWriter line_writer;
	
	if (writeSceneToLines(*typeDB, scene, line_writer, 0) == false)
	{
		logError("failed to save scene to lines");
	}
	else
	{
		auto lines = line_writer.to_lines();

		const char * path = "testScene.txt";
		const std::string basePath = Path::GetDirectory(path);
		
		if (TextIO::save(path, lines, TextIO::kLineEndings_Unix) == false)
			logError("failed to save lines to file");
		else
		{
		#if ENABLE_LOAD_AFTER_SAVE_TEST
			loadSceneFromLines_nonDestructive(lines, basePath.c_str());
		#endif
		}
	}
	
#if ENABLE_SAVE_LOAD_TIMING
	auto t2 = SDL_GetTicks();
	printf("save time: %ums\n", (t2 - t1));
#endif
}

void SceneEditor::performAction_load()
{
#if ENABLE_SAVE_LOAD_TIMING
	auto t1 = SDL_GetTicks();
#endif

	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;

// todo : store path somewhere
	const char * path = "testScene.txt";
	const std::string basePath = Path::GetDirectory(path);
	
	if (!TextIO::load(path, lines, lineEndings))
	{
		logError("failed to load text file");
	}
	else
	{
		loadSceneFromLines_nonDestructive(lines, basePath.c_str());
	}

#if ENABLE_SAVE_LOAD_TIMING
	auto t2 = SDL_GetTicks();
	printf("load time: %ums\n", (t2 - t1));
#endif
}

static std::set<std::string> s_capturedSelectedNodes; // fixme : make local var
static void backupSelectedNodes(SceneEditor & sceneEditor)
{
	Assert(s_capturedSelectedNodes.empty());
	sceneEditor.scene.assignAutoGeneratedNodeNames();
	for (auto nodeId : sceneEditor.selectedNodes)
	{
		auto & node = sceneEditor.scene.getNode(nodeId);
		s_capturedSelectedNodes.insert(node.name);
	}
}
static void restoreSelectedNodes(SceneEditor & sceneEditor)
{
	std::map<std::string, int> nodeNameToNodeId;
	for (auto & node_itr : sceneEditor.scene.nodes)
		nodeNameToNodeId.insert({ node_itr.second->name, node_itr.first });
	for (auto & nodeName : s_capturedSelectedNodes)
	{
		auto i = nodeNameToNodeId.find(nodeName);
		Assert(i != nodeNameToNodeId.end());
		sceneEditor.selectedNodes.insert(i->second);
	}
	s_capturedSelectedNodes.clear();
}

void SceneEditor::performAction_undo()
{
	if (undo.currentVersionIndex > 0)
	{
		auto & text = undo.versions[undo.currentVersionIndex - 1];
		
		std::vector<std::string> lines;
		TextIO::LineEndings lineEndings;
	
		if (!TextIO::loadText(text.c_str(), lines, lineEndings))
		{
			logError("failed to load text file");
		}
		else
		{
			Scene tempScene;
			tempScene.createRootNode();

			LineReader line_reader(lines, 0, 0);
			
			if (parseSceneFromLines(*typeDB, line_reader, "", tempScene) == false)
			{
				logError("failed to load scene from lines");
			}
			else
			{
				bool init_ok = true;
				
				for (auto node_itr : tempScene.nodes)
					init_ok &= node_itr.second->initComponents();
				
				if (init_ok == false)
				{
					tempScene.freeAllNodesAndComponents();
				}
				else
				{
					backupSelectedNodes(*this);
					{
						Assert(!deferred.isProcessing && deferred.isFlushed());
						deferred.isProcessing = true;
						{
							for (auto & node_itr : scene.nodes)
								deferred.nodesToRemove.insert(node_itr.second->id);
							
							removeNodesToRemove();
						}
						deferred.isProcessing = false;
						
						scene = tempScene;
					}
					restoreSelectedNodes(*this);
					
					undo.currentVersion = text;
					undo.currentVersionIndex--;
				}
			}
			
			tempScene.nodes.clear();
		}
	}
}

void SceneEditor::performAction_redo()
{
	if (undo.currentVersionIndex + 1 < undo.versions.size())
	{
		auto & text = undo.versions[undo.currentVersionIndex + 1];
		
		std::vector<std::string> lines;
		TextIO::LineEndings lineEndings;
	
		if (!TextIO::loadText(text.c_str(), lines, lineEndings))
		{
			logError("failed to load text file");
		}
		else
		{
			Scene tempScene;
			tempScene.createRootNode();
			
			LineReader line_reader(lines, 0, 0);

			if (parseSceneFromLines(*typeDB, line_reader, "", tempScene) == false)
			{
				logError("failed to load scene from lines");
				tempScene.nodes.clear();
			}
			else
			{
				bool init_ok = true;
				
				for (auto node_itr : tempScene.nodes)
					init_ok &= node_itr.second->initComponents();
				
				if (init_ok == false)
				{
					tempScene.freeAllNodesAndComponents();
				}
				else
				{
					backupSelectedNodes(*this);
					{
						Assert(!deferred.isProcessing && deferred.isFlushed());
						deferred.isProcessing = true;
						{
							for (auto & node_itr : scene.nodes)
								deferred.nodesToRemove.insert(node_itr.second->id);
							
							removeNodesToRemove();
						}
						deferred.isProcessing = false;
						
						scene = tempScene;
					}
					restoreSelectedNodes(*this);
					
					undo.currentVersion = text;
					undo.currentVersionIndex++;
					
				#if false // todo : remove. this is an A-B test to see if the results of a save-load-save are equal
					std::string temp;
					undoCapture(temp);
					{
						std::vector<std::string> lines;
						TextIO::LineEndings lineEndings;
						TextIO::loadText(temp.c_str(), lines, lineEndings);
						TextIO::save("version-old.txt", lines, TextIO::kLineEndings_Unix);
					}
					{
						std::vector<std::string> lines;
						TextIO::LineEndings lineEndings;
						TextIO::loadText(text.c_str(), lines, lineEndings);
						TextIO::save("version-new.txt", lines, TextIO::kLineEndings_Unix);
					}
					Assert(text == temp);
				#endif
				}
			}
			
			tempScene.nodes.clear();
		}
	}
}

void SceneEditor::performAction_copy(const bool deep)
{
	if (deep)
		performAction_copySceneNodeTrees();
	else
		performAction_copySceneNodes();
}

void SceneEditor::performAction_copySceneNodes()
{
	if (selectedNodes.empty() == false)
	{
		bool result = true;
		
		LineWriter line_writer;
		
		int indent = 0;
		
		for (auto nodeId : selectedNodes)
		{
			line_writer.append_indented_line(indent, "sceneNode");
			
			indent++;
			{
				auto & node = scene.getNode(nodeId);
				
				result &= copySceneNodeToLines(*typeDB, node, line_writer, indent);
			}
			indent--;
		}
	
		if (result)
		{
			const std::string text = line_writer.to_string();
			SDL_SetClipboardText(text.c_str());
		}
	}
}

void SceneEditor::performAction_copySceneNodeTrees()
{
	if (selectedNodes.empty() == false)
	{
		bool result = true;
		
		LineWriter line_writer;
		
		int indent = 0;
		
		for (auto nodeId : selectedNodes)
		{
			line_writer.append_indented_line(indent, "sceneNodeTree");
			
			indent++;
			{
				auto & node = scene.getNode(nodeId);
				
				result &= copySceneNodeTreeToLines(*typeDB, scene, node.id, line_writer, indent);
			}
			indent--;
		}
	
		if (result)
		{
			const std::string text = line_writer.to_string();
			SDL_SetClipboardText(text.c_str());
		}
	}
}

void SceneEditor::performAction_paste(const int parentNodeId)
{
	deferredBegin();
	{
		bool result = true;
		
		// fetch clipboard text
		
		const char * text = SDL_GetClipboardText();
	
		if (text != nullptr)
		{
			// convert text to lines
			
			std::vector<std::string> lines;
			TextIO::LineEndings lineEndings;
			if (TextIO::loadText(text, lines, lineEndings) == false)
				logError("failed to load text");
			else
			{
				LineReader line_reader(lines, 0, 0);
			
				// iterate over clipboard items
				
				for (;;)
				{
					const char * id = line_reader.get_next_line(true);
					
					if (id == nullptr || id[0] == '\t')
						break;

					if (strcmp(id, "sceneNode") == 0)
					{
						line_reader.push_indent();
						{
							result &= pasteNodeFromText(parentNodeId, line_reader);
						}
						line_reader.pop_indent();
					}
					else if (strcmp(id, "sceneNodeTree") == 0)
					{
						line_reader.push_indent();
						{
							result &= pasteNodeTreeFromText(parentNodeId, line_reader);
						}
						line_reader.pop_indent();
					}
					else
					{
						logError("unknown clipboard item: %s", id);
						result &= false;
						
						line_reader.push_indent();
						{
							line_reader.skip_current_section();
						}
						line_reader.pop_indent();
					}
				}
			}
			
			// free clipboard text
			
			SDL_free((void*)text);
			text = nullptr;
		}
		
		Assert(result); // todo : make it possible for deferred blocks to discard their changes. this would make deferred blocks into something like transactions
	}
	deferredEnd();
}

void SceneEditor::performAction_duplicate()
{
	if (selectedNodes.empty() == false)
	{
		deferredBegin();
		{
			for (auto nodeId : selectedNodes)
			{
				auto & node = scene.getNode(nodeId);
				
				LineWriter line_writer;
				if (copySceneNodeToLines(*typeDB, node, line_writer, 0))
				{
					auto lines = line_writer.to_lines();
					LineReader line_reader(lines, 0, 0);
					pasteNodeFromText(node.parentId, line_reader);
				}
			}
		}
		deferredEnd();
	}
}

void SceneEditor::drawNode(const SceneNode & node) const
{
	const bool isHovered = node.id == hoverNodeId;
	const bool isSelected = selectedNodes.count(node.id) != 0;
	
	if (isHovered)
	{
		setColor(colorWhite);
		lineCube(Vec3(), Vec3(.1f, .1f, .1f));
	}
	
	setColor(isSelected ? Color(100, 100, 0) : Color(100, 100, 100));
	fillCube(Vec3(), Vec3(.1f, .1f, .1f));
	
	if (visibility.drawNodeBoundingBoxes)
	{
		const ModelComponent * modelComp = node.components.find<ModelComponent>();
		
		if (modelComp != nullptr)
		{
			const Vec3 & min = modelComp->aabbMin;
			const Vec3 & max = modelComp->aabbMax;
			
			const Vec3 position = (min + max) / 2.f;
			const Vec3 size = (max - min) / 2.f;
			
			if (isSelected)
			{
				setColor(255, 255, 0, 255);
				lineCube(position, size);
			}
			else if (isHovered)
			{
				setColor(255, 255, 255, 127);
				lineCube(position, size);
			}
			else
			{
				setColor(127, 127, 255, 127);
				lineCube(position, size);
			}
			
			if (isSelected || isHovered)
			{
				setColor(isSelected ? 255 : isHovered ? 127 : 60, 0, 0, 40);
				fillCube(position, size);
			}
		}
	}
}

void SceneEditor::drawNodes() const
{
// todo : optimize node drawing by drawing batches

	for (auto & node_itr : scene.nodes)
	{
		auto & node = *node_itr.second;
	
		gxPushMatrix();
		{
			auto * sceneNodeComp = node.components.find<SceneNodeComponent>();
			
			if (sceneNodeComp != nullptr)
			{
				gxMultMatrixf(sceneNodeComp->objectToWorld.m_v);
			}
			
			drawNode(node);
		}
		gxPopMatrix();
	}
}

void SceneEditor::drawSceneOpaque() const
{
	if (visibility.drawGroundPlane)
	{
		setLumi(200);
		fillCube(Vec3(), Vec3(100, 1, 100));
	}
	
	if (preview.drawScene)
	{
		s_modelComponentMgr.draw();
		
		s_gltfComponentMgr.draw();
	}
}

void SceneEditor::drawSceneTranslucent() const
{
}

void SceneEditor::drawEditorOpaque() const
{
}

void SceneEditor::drawEditorTranslucent() const
{
	if (visibility.drawGrid)
	{
		pushLineSmooth(true);
		gxPushMatrix();
		{
			gxScalef(100, 100, 100);
			
			setLumi(200);
			drawGrid3dLine(100, 100, 0, 2, true);
			
			setLumi(50);
			drawGrid3dLine(500, 500, 0, 2, true);
		}
		gxPopMatrix();
		popLineSmooth();
	}
	
	if (visibility.drawNodes)
	{
		pushBlend(BLEND_ADD);
		{
			drawNodes();
		}
		popBlend();
	}
}

void SceneEditor::drawEditorGizmos() const
{
#if ENABLE_TRANSFORM_GIZMOS
// todo : draw transform gizmo on top of everything
	transformGizmo.draw();
#endif
}

void SceneEditor::drawEditorSelectedNodeLabels() const
{
	int viewportSx = 0;
	int viewportSy = 0;
	framework.getCurrentViewportSize(viewportSx, viewportSy);
	
	Mat4x4 projectionMatrix;
	camera.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);

	Mat4x4 viewMatrix;
	camera.calculateViewMatrix(viewMatrix);
	
	const Mat4x4 modelViewProjection = projectionMatrix * viewMatrix;
	
	for (auto nodeId : selectedNodes)
	{
		auto & node = scene.getNode(nodeId);
		
		auto * sceneNodeComponent = node.components.find<SceneNodeComponent>();
		Assert(sceneNodeComponent != nullptr);
		
		float w;
		Vec2 p = transformToScreen(modelViewProjection, sceneNodeComponent->objectToWorld.GetTranslation(), w);
		
		if (w > 0.f)
		{
			p[1] -= 40;
			
			const char * text = sceneNodeComponent->name.c_str();
			const float textSize = 12.f;
			
			float sx;
			float sy;
			measureText(textSize, sx, sy, "%s", text);
			
			sx = (sx + 6.f) / 2.f;
			sy = 20.f / 2.f;
			
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				setColor(20, 20, 20);
				hqFillRoundedRect(p[0] - sx, p[1] - sy, p[0] + sx, p[1] + sy, 3.f);
			}
			hqEnd();
			
			hqBegin(HQ_STROKED_ROUNDED_RECTS);
			{
				setColor(60, 60, 60);
				hqStrokeRoundedRect(p[0] - sx, p[1] - sy, p[0] + sx, p[1] + sy, 3.f, 1.f);
			}
			hqEnd();
			
			setColor(200, 200, 200);
			drawText(p[0], p[1], 12, 0, 0, "%s", text);
		}
	}
}

void SceneEditor::drawEditor() const
{
	drawEditorSelectedNodeLabels();
	
	const_cast<SceneEditor*>(this)->guiContext.draw();
}

bool SceneEditor::loadSceneFromLines_nonDestructive(std::vector<std::string> & lines, const char * basePath)
{
	Scene tempScene;
	tempScene.createRootNode();

	LineReader line_reader(lines, 0, 0);
	
	if (parseSceneFromLines(*typeDB, line_reader, basePath, tempScene) == false)
	{
		logError("failed to load scene from lines");
		tempScene.nodes.clear();
		return false;
	}
	else if (!tempScene.validate())
	{
		logError("failed to validate scene");
		tempScene.nodes.clear();
		return false;
	}
	else
	{
		bool init_ok = true;
		
		for (auto node_itr : tempScene.nodes)
			init_ok &= node_itr.second->initComponents();
		
		if (init_ok == false)
		{
			tempScene.freeAllNodesAndComponents();
			tempScene.nodes.clear();
			return false;
		}
		else
		{
			deferredBegin();
			{
				for (auto & node_itr : scene.nodes)
					deferred.nodesToRemove.insert(node_itr.second->id);
			}
			deferredEnd();
			
			scene = tempScene;
			tempScene.nodes.clear();
			
			undoReset();
			
			return true;
		}
	}
}
