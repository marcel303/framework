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

// libnfd
#include "nfd.h"

// libreflection-textio
#include "lineReader.h"
#include "lineWriter.h"

// libgg
#include "Path.h"
#include "Quat.h"
#include "StringEx.h"
#include "TextIO.h"
#include "Timer.h"

#if FRAMEWORK_USE_SDL
	// libsdl
	#include <SDL2/SDL.h> // SDL_Cursor
#endif

// std
#include <set>

//

#if defined(DEBUG)
	#define DEBUG_UNDO 0
#else
	#define DEBUG_UNDO 0 // do not alter
#endif

#define ENABLE_TRANSFORM_RESTORE_ON_SCENE_UPDATE 1

// todo : remove. need a generic way to ask for a node's bounding box

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

void SceneEditor::getEditorViewport(int & x, int & y, int & sx, int & sy) const
{
	x = preview.viewportX;
	y = preview.viewportY;
	
	int viewportSx;
	int viewportSy;
	framework.getCurrentViewportSize(viewportSx, viewportSy);
	
	sx = preview.viewportSx == -1 ? viewportSx : preview.viewportSx;
	sy = preview.viewportSy == -1 ? viewportSy : preview.viewportSy;
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

void SceneEditor::deferredBegin()
{
	Assert(deferred.numActivations || deferred.isFlushed());
		
	deferred.numActivations++;
	
	// push stack item
	
	deferred.stack.emplace_back();
	auto & stackItem = deferred.stack.back();
	
	auto & deferredState = static_cast<DeferredState&>(deferred);
	
	stackItem = deferredState;
	
	deferredState = DeferredState();
}

void SceneEditor::deferredEnd(const bool commit)
{
	if (commit)
	{
		deferredEndCommit();
	}
	else
	{
		deferredEndCancel();
	}
}

void SceneEditor::deferredEndCommit()
{
	Assert(deferred.isProcessing == false);
	Assert(deferred.numActivations > 0);
	
	deferred.numActivations--;
	
	if (deferred.isFlushed() == false)
	{
		if (deferred.numActivations == 0)
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
		else
		{
			auto & stackItem = deferred.stack.back();
			
			for (auto * node : deferred.nodesToAdd)
				stackItem.nodesToAdd.insert(node);
			deferred.nodesToAdd.clear();
			
			for (auto nodeId : deferred.nodesToRemove)
				stackItem.nodesToRemove.insert(nodeId);
			deferred.nodesToRemove.clear();
			
			for (auto nodeId : deferred.nodesToSelect)
				stackItem.nodesToSelect.insert(nodeId);
			deferred.nodesToSelect.clear();
			
			Assert(deferred.isFlushed());
		}
	}
	
	// pop stack item
	
	auto & stackItem = deferred.stack.back();
	
	auto & deferredState = static_cast<DeferredState&>(deferred);
	
	deferredState = stackItem;
	
	deferred.stack.pop_back();
	
	Assert(deferred.numActivations > 0 || deferred.isFlushed());
}

void SceneEditor::deferredEndCancel()
{
	Assert(deferred.isProcessing == false);
	Assert(deferred.numActivations > 0);
	
	deferred.numActivations--;
	
	if (deferred.isFlushed() == false)
	{
		for (auto * node : deferred.nodesToAdd)
		{
			node->freeComponents();
			
			delete node;
			node = nullptr;
		}
		
		deferred.nodesToAdd.clear();
		deferred.nodesToRemove.clear();
		deferred.nodesToSelect.clear();
		
		Assert(deferred.isFlushed());
	}
	
	// pop stack item
	
	auto & stackItem = deferred.stack.back();
	
	auto & deferredState = static_cast<DeferredState&>(deferred);
	
	deferredState = stackItem;
	
	deferred.stack.pop_back();
	
	Assert(deferred.numActivations > 0 || deferred.isFlushed());
}

void SceneEditor::removeNodeSubTree(const int nodeId)
{
	AssertMsg(deferred.isProcessing, "this method may only be called by deferredEnd!");
	
	std::vector<int> removedNodeIds;
	scene.removeNodeSubTree(nodeId, &removedNodeIds);
	
	for (auto nodeId : removedNodeIds)
	{
		// update node references
		
		if (nodeId == scene.rootNodeId)
			scene.rootNodeId = -1;
		
		selection.selectedNodes.erase(nodeId);
		
		deferred.nodesToRemove.erase(nodeId);
		deferred.nodesToSelect.erase(nodeId);
	}
}

void SceneEditor::addNodesToAdd()
{
	AssertMsg(deferred.isProcessing, "this method may only be called by deferredEnd!");
	
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
	AssertMsg(deferred.isProcessing, "this method may only be called by deferredEnd!");
	
	while (!deferred.nodesToRemove.empty())
	{
		auto nodeToRemoveItr = deferred.nodesToRemove.begin();
		auto nodeId = *nodeToRemoveItr;
		
		removeNodeSubTree(nodeId);
	}
}

void SceneEditor::selectNodesToSelect(const bool append)
{
	AssertMsg(deferred.isProcessing, "this method may only be called by deferredEnd!");
	
	if (!deferred.nodesToSelect.empty())
	{
		if (append == false)
		{
			selection = Selection();
			selection.selectedNodes.clear();
		}
		
		for (auto nodeId : deferred.nodesToSelect)
		{
			selection.selectedNodes.insert(nodeId);
			nodeUi.nodeToGiveFocus = nodeId;
			
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
						
						ImGui::Reflection_Callbacks callbacks;
						callbacks.makePathRelative = [this](std::string & path)
							{
								if (!documentInfo.path.empty())
								{
									path = Path::MakeRelative(Path::GetDirectory(documentInfo.path), path);
								}
							};
						
						if (ImGui::Reflection_StructuredType(
							*typeDB,
							*componentType,
							component,
							isSet,
							nullptr,
							&changedMemberObject,
							&callbacks))
						{
							// signal the component one of its properties has changed
							component->propertyChanged(changedMemberObject);
						}
						
						ImGui::Unindent();
					}
				}
				ImGui::EndGroup();
			
			#if 1 // todo : fix issue with this menu obscuring child menus. array add/insert etc don't work anymore :-(
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
		else if (strcmp(id, "sceneNode") == 0)
		{
			clipboardInfo.hasNode = true;
		}
		else if (strcmp(id, "sceneNodeTree") == 0)
		{
			clipboardInfo.hasNodeTree = true;
		}
	}
}

bool SceneEditor::pasteNodeFromLines(const int parentId, LineReader & line_reader)
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
				deferred.nodesToAdd.insert(childNode);
				deferred.nodesToSelect.insert(childNode->id);
			}
			deferredEnd(true);
			
			return true;
		}
	}
}

bool SceneEditor::pasteNodeTreeFromLines(const int parentId, LineReader & line_reader, const bool keepRootNode)
{
	Scene childScene;
	childScene.nodeIdAllocator = &scene;
	
	// create root node, assign its parent id, and make sure it has a transform component
	childScene.createRootNode();
	auto & rootNode = childScene.getRootNode();
	rootNode.parentId = parentId;
	
	if (!pasteSceneNodeTreeFromLines(*typeDB, line_reader, childScene))
	{
		childScene.freeAllNodesAndComponents();
		return false;
	}
	
	bool init_ok = true;

	for (auto node_itr : childScene.nodes)
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
		childScene.freeAllNodesAndComponents();
		return false;
	}
	else
	{
		deferredBegin();
		{
			for (auto node_itr : childScene.nodes)
			{
				auto * node = node_itr.second;
				
				if (keepRootNode == false)
				{
					// re-parent the node if it is the root node
					
					if (node->id == rootNode.id)
					{
						node->parentId = parentId;
						deferred.nodesToSelect.insert(node->id);
					}
				}
				else
				{
					// skip this node if its the root node
					
					if (node->id == rootNode.id)
					{
						node->freeComponents();
						continue;
					}
					
					// re-parent the node if it was parented to the root node
					
					if (node->parentId == rootNode.id)
					{
						node->parentId = parentId;
						deferred.nodesToSelect.insert(node->id);
					}
				}
				
				deferred.nodesToAdd.insert(node);
			}
		}
		deferredEnd(true);
		
		childScene.nodes.clear();
		
		return true;
	}
}

SceneEditor::NodeStructureEditingAction SceneEditor::doNodeStructureContextMenu(SceneNode & node)
{
	//logDebug("context window for %d", node.id);
	
	NodeStructureEditingAction result = kNodeStructureEditingAction_None;

	auto * sceneNodeComponent = node.components.find<SceneNodeComponent>();
	
	char name[256];
	strcpy_s(name, sizeof(name), sceneNodeComponent->name.c_str());
	if (ImGui::InputText("Name", name, sizeof(name)))
	{
		sceneNodeComponent->name = name;
	}
	
	// note : for actions that change the scene structure, we defer changes until after showing the node structure UI. this ensures node ids remain valid during scene traversal. we could use a global deferred update begin/end scope surrounding node traversal as well, but this seems more clean and easier to grasp to me

	if (ImGui::MenuItem("Copy"))
	{
		result = kNodeStructureEditingAction_NodeCopy;
	}
	
	if (ImGui::MenuItem("Copy tree"))
	{
		result = kNodeStructureEditingAction_NodeCopyTree;
	}
	
	if (ImGui::MenuItem("Paste as child", nullptr, false, clipboardInfo.hasNode || clipboardInfo.hasNodeTree))
	{
		result = kNodeStructureEditingAction_NodePasteChild;
	}
	
	if (ImGui::MenuItem("Paste as sibling", nullptr, false, (clipboardInfo.hasNode || clipboardInfo.hasNodeTree) && node.parentId != -1))
	{
		result = kNodeStructureEditingAction_NodePasteSibling;
	}
	
	if (ImGui::MenuItem("Remove", nullptr, false, node.parentId != -1))
	{
		result = kNodeStructureEditingAction_NodeRemove;
	}

	if (ImGui::MenuItem("Add child node"))
	{
		result = kNodeStructureEditingAction_NodeAddChild;
	}
	
	if (ImGui::MenuItem("Import nodes from scene"))
	{
		result = kNodeStructureEditingAction_NodeSceneImport;
	}
	
	if (ImGui::MenuItem("Attach scene"))
	{
		result = kNodeStructureEditingAction_NodeSceneAttach;
	}
	
	if (ImGui::MenuItem("Update attached scene", nullptr, false, sceneNodeComponent->attachedFromScene.empty() == false))
	{
		result = kNodeStructureEditingAction_NodeSceneAttachUpdate;
	}
	
	if (ImGui::MenuItem("Focus camera"))
	{
		switch (camera.mode)
		{
		case Camera::kMode_Orbit:
			camera.orbit.origin = sceneNodeComponent->objectToWorld.GetTranslation();
			break;
		case Camera::kMode_Ortho:
			camera.ortho.position = sceneNodeComponent->objectToWorld.GetTranslation();
			break;
		case Camera::kMode_FirstPerson:
			{
				Mat4x4 cameraTransform;
				camera.firstPerson.calculateWorldMatrix(cameraTransform);
				
				const Vec3 delta = sceneNodeComponent->objectToWorld.GetTranslation() - cameraTransform.GetTranslation();
				
				const float radToDeg = 180.f / float(M_PI);
				
				camera.firstPerson.yaw = -atan2f(delta[0], delta[2]) * radToDeg;
				camera.firstPerson.pitch = atan2f(delta[1], hypotf(delta[0], delta[2])) * radToDeg;
				camera.firstPerson.roll = 0.f;
			}
			break;
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

				framework.setClipboardText(text.c_str());
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
		LineReader line_reader(
			clipboardInfo.lines,
			clipboardInfo.component_lineIndex,
			clipboardInfo.component_lineIndent);
		
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
		LineReader line_reader(
			clipboardInfo.lines,
			clipboardInfo.component_lineIndex,
			clipboardInfo.component_lineIndent);
		
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

SceneEditor::NodeStructureEditingAction SceneEditor::editNodeStructure_traverse(const int nodeId)
{
	NodeStructureEditingAction result = kNodeStructureEditingAction_None;
	
	const bool do_filter = nodeUi.nodeDisplayNameFilter[0] != 0;
	
	auto & node = scene.getNode(nodeId);
	
	//
	
	const bool isSelected = selection.selectedNodes.count(node.id) != 0;
	const bool isLeaf = node.childNodeIds.empty();
	const bool isRoot = node.parentId == -1;
	
	ImGui::PushID(&node);
	{
		// update scrolling
		
		if (nodeUi.nodeToGiveFocus == nodeId)
		{
			nodeUi.nodeToGiveFocus = -1;
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
		
		const bool isReference = sceneNodeComponent && !sceneNodeComponent->attachedFromScene.empty();
		
		const bool isOpen = ImGui::TreeNodeEx(&node,
			(ImGuiTreeNodeFlags_OpenOnArrow * 1) |
			(ImGuiTreeNodeFlags_Selected * isSelected) |
			(ImGuiTreeNodeFlags_Leaf * isLeaf) |
			(ImGuiTreeNodeFlags_DefaultOpen * isRoot) |
			(ImGuiTreeNodeFlags_Framed * isReference) | // give attached scenes a different visual appearance
			(ImGuiTreeNodeFlags_FramePadding * 0 |
			(ImGuiTreeNodeFlags_NavLeftJumpsBackHere * 1)), "%s", name);
		
		const bool isClicked = ImGui::IsItemClicked(0) || ImGui::IsItemClicked(1);
		
		if (isClicked)
		{
			if (!ImGui::GetIO().KeyShift)
				selection = Selection();
			selection.selectedNodes.insert(node.id);
		}
	
		if (ImGui::BeginPopupContextItem("NodeStructureMenu"))
		{
			const NodeStructureEditingAction menuResult = doNodeStructureContextMenu(node);
			
			if (menuResult != kNodeStructureEditingAction_None)
			{
				result = menuResult;
			}

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

				const NodeStructureEditingAction childResult = editNodeStructure_traverse(childNodeId);
				
				if (childResult != kNodeStructureEditingAction_None)
				{
					result = childResult;
				}
			}
		
			ImGui::TreePop();
		}
	}
	ImGui::PopID();
	
	return result;
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
		
		for (auto nodeId : selection.selectedNodes)
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
		deferred.nodesToAdd.insert(node);
		deferred.nodesToSelect.insert(node->id);
	}
	deferredEnd(true);
	
	return node->id;
}

int SceneEditor::addNodesFromScene(const char * path, const int parentId)
{
	Scene childScene;
	childScene.nodeIdAllocator = &scene;
	
	// 1. create root node
	
	childScene.createRootNode();
	auto & rootNode = childScene.getRootNode();
	
	// 2.1 assign its parent id
	
	rootNode.parentId = parentId;
	
	// 2.2 assign its display name
	
	auto * sceneNodeComponent = s_sceneNodeComponentMgr.getComponent(rootNode.components.id);
	
	sceneNodeComponent->name = Path::GetBaseName(path);
	
	// 3. and make sure it has a transform component
	
	auto * transformComponent = s_transformComponentMgr.createComponent(rootNode.components.id);
	rootNode.components.add(transformComponent);
	
	// 4. load scene from file
	
	if (!parseSceneFromFile(*typeDB, path, childScene))
	{
		childScene.freeAllNodesAndComponents();
		return -1;
	}
	
	// 5.1. update file paths for the imported nodes
	
	updateFilePaths(
		childScene,
		typeDB,
		Path::GetDirectory(path).c_str(),
		Path::GetDirectory(documentInfo.path).c_str());
	
	// 5.2. anonymize node names. this to ensure nodes get assigned a new unique auto-generated name on save
	
	for (auto node_itr : childScene.nodes)
	{
		node_itr.second->name.clear();
	}
	
	// 6. initialize the scene
	
	bool init_ok = true;
	
	for (auto node_itr : childScene.nodes)
		init_ok &= node_itr.second->initComponents();

	if (init_ok == false)
	{
		childScene.freeAllNodesAndComponents();
		return -1;
	}
	else
	{
		auto & rootNode = childScene.getRootNode();
		
		// if all went well, move the nodes to the parent scene
		
		deferredBegin();
		{
			for (auto node_itr : childScene.nodes)
			{
				deferred.nodesToAdd.insert(node_itr.second);
			}
			
			deferred.nodesToSelect.insert(childScene.rootNodeId);
			
			childScene.nodes.clear();
		}
		deferredEnd(true);
		
		return childScene.rootNodeId;
	}
}

int SceneEditor::attachScene(const char * path, const int parentId)
{
	bool success = true;
	
	int rootNodeId = -1;
	
	deferredBegin();
	{
		// add nodes
		
		rootNodeId = addNodesFromScene(path, parentId);
		
		success &= rootNodeId != -1;
		
		// mark sub-tree as being attached from scene
		
		if (rootNodeId != -1)
		{
			SceneNode * rootNode = nullptr;
			for (auto * node : deferred.nodesToAdd)
				if (node->id == rootNodeId)
					rootNode = node;
			if (rootNode == nullptr)
				rootNode = &scene.getNode(rootNodeId);
			
			Assert(rootNode != nullptr);
				
			auto * sceneNodeComponent = s_sceneNodeComponentMgr.getComponent(rootNode->components.id);
		
			sceneNodeComponent->attachedFromScene = path;
		}
	}
	deferredEnd(success);
	
	return rootNodeId;
}

int SceneEditor::updateAttachedScene(const int rootNodeId)
{
	auto & rootNode = scene.getNode(rootNodeId);
	auto * sceneNodeComponent = rootNode.components.find<SceneNodeComponent>();
	
	bool success = true;
	
#if ENABLE_TRANSFORM_RESTORE_ON_SCENE_UPDATE
	// todo : should keep the root node in-tact? (transform)
	// todo : even better would be if we supported overrides for node components in the sub-tree. note : this may require a full refactor, where we make the scene editor into a fancy text editor for template files, and figure out some way to keep the text and run-time scene representations in sync. although.. I like how currently attached scenes can be fully modified. perhaps another approach would be to serialize the attached scene, remove it, re-add it, and attempt to re-apply to serialized data (matching a 'origin-node-id' stored inside the imported nodes' scene node component)
	
	LineWriter line_writer;
	auto * oldTransformComponent = rootNode.components.find<TransformComponent>();
	bool hasOldTransform = false;
	
	if (success && oldTransformComponent != nullptr)
	{
		success &= writeComponentToLines(
			*typeDB,
			*oldTransformComponent,
			line_writer,
			0);
		
		if (success)
		{
			hasOldTransform = true;
		}
	}
#endif
	
	const int parentNodeId = rootNode.parentId;
	const std::string path = sceneNodeComponent->attachedFromScene;
	
	int newRootNodeId = -1;

	deferredBegin();
	{
		// remove sub-tree
		
		deferred.nodesToRemove.insert(rootNodeId);
		
		// add nodes from scene
		
		newRootNodeId = attachScene(path.c_str(), parentNodeId);
		
		success &= newRootNodeId != -1;
	
	#if ENABLE_TRANSFORM_RESTORE_ON_SCENE_UPDATE
		if (success && hasOldTransform)
		{
			SceneNode * newRootNode = nullptr;
			for (auto * node : deferred.nodesToAdd)
				if (node->id == newRootNodeId)
					newRootNode = node;
			
			auto * newTransformComponent = newRootNode->components.find<TransformComponent>();
			if (newTransformComponent != nullptr)
			{
				auto lines = line_writer.to_lines();
				LineReader line_reader(lines, 1, 1);
				success &= parseComponentFromLines(
					*typeDB,
					line_reader,
					*newTransformComponent);
			}
		}
	#endif
	}
	deferredEnd(success);

	return newRootNodeId;
}

void SceneEditor::tickGui(const float dt, bool & inputIsCaptured)
{
	updateClipboardInfo();
	
	int viewSx;
	int viewSy;
	framework.getCurrentViewportSize(viewSx, viewSy);
	
	guiContext.processBegin(dt, viewSx, viewSy, inputIsCaptured);
	{
		if (showUi)
		{
			ImGui::SetNextWindowPos(ImVec2(0, 20));
			ImGui::SetNextWindowSize(ImVec2(kMainWindowWidth, viewSy - 20));
			if (ImGui::Begin("Editor", nullptr,
				ImGuiWindowFlags_NoResize |
				ImGuiWindowFlags_NoMove |
				ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_NoTitleBar))
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
						nodeUi.nodesToOpen_active = nodeUi.nodesToOpen_deferred;
						nodeUi.nodesToOpen_deferred.clear();
						
						const NodeStructureEditingAction action = editNodeStructure_traverse(scene.rootNodeId);
						
						switch (action)
						{
						case kNodeStructureEditingAction_None:
							break;
							
						case kNodeStructureEditingAction_NodeCopy:
							performAction_copy(false);
							break;
						case kNodeStructureEditingAction_NodeCopyTree:
							performAction_copy(true);
							break;
							
						case kNodeStructureEditingAction_NodePasteChild:
							performAction_pasteChild();
							break;
						case kNodeStructureEditingAction_NodePasteSibling:
							performAction_pasteSibling();
							break;
							
						case kNodeStructureEditingAction_NodeRemove:
							deferredBegin();
							{
								for (auto nodeId : selection.selectedNodes)
									deferred.nodesToRemove.insert(nodeId);
							}
							deferredEnd(true);
							break;
							
						case kNodeStructureEditingAction_NodeAddChild:
							performAction_addChild();
							break;
							
						case kNodeStructureEditingAction_NodeSceneAttach:
							performAction_sceneAttach();
							break;
						case kNodeStructureEditingAction_NodeSceneAttachUpdate:
							performAction_sceneAttachUpdate();
							break;
							
						case kNodeStructureEditingAction_NodeSceneImport:
							performAction_sceneImport();
							break;
						}
					}
					ImGui::EndChild();
					
					if (ImGui::IsItemHovered())
					{
						if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)) ||
							ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
						{
							deferredBegin();
							{
								for (auto nodeId : selection.selectedNodes)
									deferred.nodesToRemove.insert(nodeId);
							}
							deferredEnd(true);
						}
					}
				}
				
				// node editing
				
				if (ImGui::CollapsingHeader("Selected node(s)", ImGuiTreeNodeFlags_DefaultOpen))
				{
					ImGui::BeginChild("Selected nodes", ImVec2(0, 300), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
					{
						ImGui::PushItemWidth(180.f);
						for (auto & selectedNodeId : selection.selectedNodes)
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
				if (ImGui::MenuItem("Load"))
				{
					nfdchar_t * path = nullptr;
					
					if (NFD_OpenDialog(nullptr, nullptr, &path) == NFD_OKAY)
					{
						performAction_load(path);
					}
				
					if (path != nullptr)
					{
						free(path);
						path = nullptr;
					}
				}
				
				if (ImGui::MenuItem("Reload"))
				{
					if (!documentInfo.path.empty())
					{
						const std::string path = documentInfo.path;
						
						performAction_load(path.c_str());
					}
				}
				
				if (ImGui::MenuItem("Save"))
				{
					performAction_save();
				}
				
				if (ImGui::MenuItem("Save as.."))
				{
					nfdchar_t * path = nullptr;
					
					if (NFD_SaveDialog(nullptr, nullptr, &path) == NFD_OKAY)
					{
						performAction_save(path, true);
					}
				
					if (path != nullptr)
					{
						free(path);
						path = nullptr;
					}
				}
				
				ImGui::EndMenu();
			}
			
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", nullptr, false, undo.currentVersionIndex > 0))
					performAction_undo();
				if (ImGui::MenuItem("Redo", nullptr, false, undo.currentVersionIndex + 1 < undo.versions.size()))
					performAction_redo();
				ImGui::Separator();
				
				if (ImGui::MenuItem("Copy", nullptr, false, !selection.selectedNodes.empty()))
					performAction_copy(false);
				if (ImGui::MenuItem("Copy tree", nullptr, false, !selection.selectedNodes.empty()))
					performAction_copy(true);
				if (ImGui::MenuItem("Paste", nullptr, false, clipboardInfo.hasNode || clipboardInfo.hasNodeTree))
					performAction_paste(scene.rootNodeId);
				ImGui::Separator();
				
				if (ImGui::MenuItem("Duplicate", nullptr, false, !selection.selectedNodes.empty()))
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
}

void SceneEditor::tickView(const float dt, bool & inputIsCaptured)
{
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
	
	Vec3 pointerOrigin_world;
	Vec3 pointerDirection_world;
	bool hasPointer = false;
	bool pointerIsActive = false;
	bool pointerBecameActive = false;
	
	if (framework.isStereoVr())
	{
		if (vrPointer[0].hasTransform)
		{
			const Mat4x4 transform = vrPointer[0].getTransform(framework.vrOrigin);
			pointerOrigin_world = transform.GetTranslation();
			pointerDirection_world = transform.GetAxis(2);
			pointerIsActive = vrPointer[0].isDown(VrButton_Trigger);
			pointerBecameActive = vrPointer[0].wentDown(VrButton_Trigger);
			hasPointer = true;
		}
	}
	else
	{
		int viewportX;
		int viewportY;
		int viewportSx;
		int viewportSy;
		getEditorViewport(viewportX, viewportY, viewportSx, viewportSy);
		
		Mat4x4 projectionMatrix;
		camera.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);
		
		Mat4x4 cameraToWorld;
		camera.calculateWorldMatrix(cameraToWorld);
		
		if (camera.mode == Camera::kMode_Ortho)
		{
			const Vec2 mousePosition_screen(
				mouse.x,
				mouse.y);
			const Vec3 mousePosition_clip(
				(mousePosition_screen[0] - viewportX) / float(viewportSx) * 2.f - 1.f,
				(mousePosition_screen[1] - viewportY) / float(viewportSy) * 2.f - 1.f,
				0.f);
			Vec3 mousePosition_camera = projectionMatrix.CalcInv().Mul4(mousePosition_clip);
			mousePosition_camera[1] = -mousePosition_camera[1];
			
			pointerOrigin_world = cameraToWorld.Mul4(mousePosition_camera) - cameraToWorld.GetAxis(2) * 10.f;
			
			pointerDirection_world = cameraToWorld.GetAxis(2);
			pointerDirection_world = pointerDirection_world.CalcNormalized();
		}
		else
		{
			pointerOrigin_world = cameraToWorld.GetTranslation();
			
			const Vec2 mousePosition_screen(
				mouse.x,
				mouse.y);
			const Vec2 mousePosition_clip(
				(mousePosition_screen[0] - viewportX) / float(viewportSx) * 2.f - 1.f,
				(mousePosition_screen[1] - viewportY) / float(viewportSy) * 2.f - 1.f);
			Vec2 mousePosition_camera = projectionMatrix.CalcInv().Mul4(mousePosition_clip);
			
			pointerDirection_world = cameraToWorld.Mul3(
				Vec3(
					+mousePosition_camera[0],
					-mousePosition_camera[1],
					1.f));
			pointerDirection_world = pointerDirection_world.CalcNormalized();
		}
		
		pointerIsActive = mouse.isDown(BUTTON_LEFT);
		pointerBecameActive = mouse.wentDown(BUTTON_LEFT);
		hasPointer = true;
	}
	
#if ENABLE_TRANSFORM_GIZMOS
	// 2. transformation gizo interaction
	
	if (selection.selectedNodes.size() != 1)
	{
		transformGizmo.hide();
	}
	else
	{
		auto & node = scene.getNode(*selection.selectedNodes.begin());
		
		// determine the current global transform
		
		Mat4x4 globalTransform(true);
		
		auto * sceneNodeComponent = node.components.find<SceneNodeComponent>();
		Assert(sceneNodeComponent != nullptr);
		
		if (sceneNodeComponent != nullptr)
			globalTransform = sceneNodeComponent->objectToWorld;
		
		// let the gizmo do it's thing
		
		transformGizmo.show(globalTransform);
		
		if (hasPointer &&
			transformGizmo.tick(
				pointerOrigin_world,
				pointerDirection_world,
				pointerIsActive,
				pointerBecameActive,
				inputIsCaptured))
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
		inputIsCaptured == false && hasPointer
		? raycast(pointerOrigin_world, pointerDirection_world, selection.selectedNodes)
		: nullptr;
	
	hoverNodeId = hoverNode ? hoverNode->id : -1;
	
	// 4. update mouse cursor

// todo : framework should mediate here. perhaps have a requested mouse cursor and update it each frame. or make it a member of the mouse, and add an explicit mouse.updateCursor() method

	if (inputIsCaptured == false)
	{
	#if FRAMEWORK_USE_SDL
		static SDL_Cursor * cursorHand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		SDL_SetCursor(hoverNode == nullptr ? SDL_GetDefaultCursor() : cursorHand);
	#endif
	}
	
	if (inputIsCaptured == false && hasPointer && pointerBecameActive)
	{
		// action: add node from template. todo : this is a test action. remove! instead, have a template list or something and allow adding instances from there
		if (keyboard.isDown(SDLK_RSHIFT))
		{
			inputIsCaptured = true;
			
			// todo : make action dependent on editing mode. in this case, 'editing mode = node placement'. for the vr version of the app, it would be nice to have a selection wheel to select the editing mode
		
			// find intersection point with the ground plane
		
			if (pointerDirection_world[1] != 0.f)
			{
				const float t = -pointerOrigin_world[1] / pointerDirection_world[1];
				
				if (t >= 0.f)
				{
					const Vec3 groundPosition = pointerOrigin_world + pointerDirection_world * t;
					
					if (keyboard.isDown(SDLK_c))
					{
						bool success = true;
						
						deferredBegin();
						{
							for (auto & parentNodeId : selection.selectedNodes)
							{
								auto & parentNode = scene.getNode(parentNodeId);
								
								auto * sceneNodeComp = parentNode.components.find<SceneNodeComponent>();
								
								Assert(sceneNodeComp != nullptr);
								if (sceneNodeComp != nullptr)
								{
									const Vec3 groundPosition_parent = sceneNodeComp->objectToWorld.CalcInv().Mul4(groundPosition);
									
									const int rootNodeId = addNodeFromTemplate_v2(groundPosition_parent, AngleAxis(), parentNodeId);
									
									success &= rootNodeId != -1;
								}
							}
						}
						deferredEnd(success);
					}
					else
					{
						bool success = true;
						
						deferredBegin();
						{
							const int rootNodeId = addNodeFromTemplate_v2(groundPosition, AngleAxis(), scene.rootNodeId);
							
							success &= rootNodeId != -1;
						}
						deferredEnd(success);
					}
				}
			}
		}
		else
		{
			if (!modShift())
				selection = Selection();
			
			if (hoverNode != nullptr)
			{
				selection.selectedNodes.insert(hoverNode->id);
				markNodeOpenUntilRoot(hoverNode->id);
				nodeUi.nodeToGiveFocus = hoverNode->id;
			}
		}
	}
	
	// action: remove selected node(s)
	if (inputIsCaptured == false && (keyboard.wentDown(SDLK_BACKSPACE) || keyboard.wentDown(SDLK_DELETE)))
	{
		inputIsCaptured = true;
		
		deferredBegin();
		{
			for (auto nodeId : selection.selectedNodes)
			{
				if (nodeId == scene.rootNodeId)
					continue;
				
				deferred.nodesToRemove.insert(nodeId);
			}
		}
		deferredEnd(true);
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
	for (auto & selectedNodeId : selection.selectedNodes)
		Assert(scene.nodes.count(selectedNodeId) != 0);
	Assert(hoverNodeId == -1 || scene.nodes.count(hoverNodeId) != 0);
	Assert(nodeUi.nodeToGiveFocus == -1 || scene.nodes.count(nodeUi.nodeToGiveFocus) != 0);
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
	if (documentInfo.path.empty())
	{
		nfdchar_t * path = nullptr;
					
		if (NFD_SaveDialog(nullptr, nullptr, &path) == NFD_OKAY)
		{
			performAction_save(path, true);
		}

		if (path != nullptr)
		{
			free(path);
			path = nullptr;
		}
	}
	else
	{
		performAction_save(documentInfo.path.c_str(), false);
	}
}

void SceneEditor::performAction_save(const char * path, const bool updateDocumentPath)
{
#if ENABLE_SAVE_LOAD_TIMING
	auto t1 = g_TimerRT.TimeUS_get();
#endif
	
	LineWriter line_writer;
	
	if (writeSceneToLines(*typeDB, scene, line_writer, 0) == false)
	{
		logError("failed to save scene to lines");
	}
	else
	{
		auto lines = line_writer.to_lines();
		
		if (TextIO::save(path, lines, TextIO::kLineEndings_Unix) == false)
			logError("failed to save lines to file");
		else
		{
		#if ENABLE_LOAD_AFTER_SAVE_TEST
			const std::string basePath = Path::GetDirectory(path);
			
			loadSceneFromLines_nonDestructive(lines, basePath.c_str());
		#endif
		
			if (updateDocumentPath)
			{
				// update file paths so they become relative to the new document path
				
				updateFilePaths(
					scene,
					typeDB,
					Path::GetDirectory(documentInfo.path).c_str(),
					Path::GetDirectory(path).c_str());
				
				// remember path as current path
								
				documentInfo.path = path;
			}
		}
	}
	
#if ENABLE_SAVE_LOAD_TIMING
	auto t2 = g_TimerRT.TimeUS_get();
	logDebug("save time: %ums", (t2 - t1));
#endif
}

void SceneEditor::performAction_load(const char * path)
{
#if ENABLE_SAVE_LOAD_TIMING
	auto t1 = g_TimerRT.TimeUS_get();
#endif

	resetDocument();
	
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (!TextIO::load(path, lines, lineEndings))
	{
		logError("failed to load text file");
	}
	else
	{
		const std::string basePath = Path::GetDirectory(path);
		
		if (loadSceneFromLines_nonDestructive(lines, basePath.c_str()))
		{
			// remember path as current path
							
			documentInfo.path = path;
		}
	}

#if ENABLE_SAVE_LOAD_TIMING
	auto t2 = g_TimerRT.TimeUS_get();
	logDebug("load time: %ums", (t2 - t1));
#endif
}

static std::set<std::string> s_capturedSelectedNodes; // fixme : make local var
static void backupSelectedNodes(SceneEditor & sceneEditor)
{
	Assert(s_capturedSelectedNodes.empty());
	sceneEditor.scene.assignAutoGeneratedNodeNames();
	for (auto nodeId : sceneEditor.selection.selectedNodes)
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
		sceneEditor.selection.selectedNodes.insert(i->second);
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
					
				#if DEBUG_UNDO
					// this is an A-B test to see if the results of a save-load-save are equal
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

void SceneEditor::performAction_copy(const bool deep) const
{
	if (deep)
		performAction_copySceneNodeTrees();
	else
		performAction_copySceneNodes();
}

void SceneEditor::performAction_copySceneNodes() const
{
	if (selection.selectedNodes.empty() == false)
	{
		bool result = true;
		
		LineWriter line_writer;
		
		int indent = 0;
		
		for (auto nodeId : selection.selectedNodes)
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
			
			framework.setClipboardText(text.c_str());
		}
	}
}

void SceneEditor::performAction_copySceneNodeTrees() const
{
	if (selection.selectedNodes.empty() == false)
	{
		bool result = true;
		
		LineWriter line_writer;
		
		int indent = 0;
		
		for (auto nodeId : selection.selectedNodes)
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

			framework.setClipboardText(text.c_str());
		}
	}
}

bool SceneEditor::performAction_pasteChild()
{
	bool success = true;
	
	deferredBegin();
	{
		for (auto nodeId : selection.selectedNodes)
			success &= performAction_paste(nodeId);
	}
	deferredEnd(success);
	
	return success;
}

bool SceneEditor::performAction_pasteSibling()
{
	bool success = true;
	
	deferredBegin();
	{
		for (auto nodeId : selection.selectedNodes)
		{
			auto & node = scene.getNode(nodeId);
			success &= performAction_paste(node.parentId);
		}
	}
	deferredEnd(success);
	
	return success;
}

bool SceneEditor::performAction_paste(const int parentNodeId)
{
	bool success = true;
	
	deferredBegin();
	{
		// fetch clipboard text

		const std::string clipboard_text = framework.getClipboardText();
	
		if (!clipboard_text.empty())
		{
			// convert text to lines
			
			std::vector<std::string> lines;
			TextIO::LineEndings lineEndings;
			if (TextIO::loadText(clipboard_text.c_str(), lines, lineEndings) == false)
			{
				logError("failed to load clipboard text");
				success &= false;
			}
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
							success &= pasteNodeFromLines(parentNodeId, line_reader);
						}
						line_reader.pop_indent();
					}
					else if (strcmp(id, "sceneNodeTree") == 0)
					{
						line_reader.push_indent();
						{
							success &= pasteNodeTreeFromLines(parentNodeId, line_reader, false);
						}
						line_reader.pop_indent();
					}
					else
					{
						logError("unknown clipboard item: %s", id);
						success &= false;
						
						line_reader.push_indent();
						{
							line_reader.skip_current_section();
						}
						line_reader.pop_indent();
					}
				}
			}
		}
		
		Assert(success);
	}
	deferredEnd(success);
	
	return success;
}

bool SceneEditor::performAction_addChild()
{
	bool success = true;
	
	deferredBegin();
	{
		for (auto nodeId : selection.selectedNodes)
		{
			success &= performAction_addChild(nodeId);
		}
	}
	deferredEnd(success);
	
	return success;
}

bool SceneEditor::performAction_addChild(const int parentNodeId)
{
	bool success = true;
	
	SceneNode * childNode = new SceneNode();
	childNode->id = scene.allocNodeId();
	childNode->parentId = parentNodeId;
	
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
		
		success &= false;
	}
	else
	{
		deferredBegin();
		{
			deferred.nodesToAdd.insert(childNode);
			deferred.nodesToSelect.insert(childNode->id);
		}
		deferredEnd(success);
	}
	
	return success;
}

bool SceneEditor::performAction_sceneAttach()
{
	bool success = true;
	
	nfdchar_t * path = nullptr;

	if (NFD_OpenDialog(nullptr, nullptr, &path) == NFD_OKAY)
	{
		deferredBegin();
		{
			for (auto nodeId : selection.selectedNodes)
				success &= attachScene(path, nodeId) != -1;
		}
		deferredEnd(success);
	}

	if (path != nullptr)
	{
		free(path);
		path = nullptr;
	}
	
	return success;
}

bool SceneEditor::performAction_sceneAttachUpdate()
{
	bool success = true;
	
	deferredBegin();
	{
		for (auto nodeId : selection.selectedNodes)
			success &= updateAttachedScene(nodeId) != -1;
	}
	deferredEnd(success);
	
	return success;
}

bool SceneEditor::performAction_sceneImport()
{
	bool success = true;
	
	nfdchar_t * path = nullptr;

	if (NFD_OpenDialog(nullptr, nullptr, &path) == NFD_OKAY)
	{
		deferredBegin();
		{
			for (auto nodeId : selection.selectedNodes)
				success &= addNodesFromScene(path, nodeId) != -1;
		}
		deferredEnd(success);
	}

	if (path != nullptr)
	{
		free(path);
		path = nullptr;
	}
	
	return success;
}

bool SceneEditor::performAction_duplicate()
{
	bool success = true;
	
	if (selection.selectedNodes.empty() == false)
	{
		deferredBegin();
		{
			for (auto nodeId : selection.selectedNodes)
			{
				auto & node = scene.getNode(nodeId);
				
			#if true
				LineWriter line_writer;
				if (copySceneNodeTreeToLines(*typeDB, scene, node.id, line_writer, 0))
				{
					auto lines = line_writer.to_lines();
					LineReader line_reader(lines, 0, 0);
					success &= pasteNodeTreeFromLines(node.parentId, line_reader, false);
				}
			#else
				LineWriter line_writer;
				if (copySceneNodeToLines(*typeDB, node, line_writer, 0))
				{
					auto lines = line_writer.to_lines();
					LineReader line_reader(lines, 0, 0);
					success &= pasteNodeFromLines(node.parentId, line_reader);
				}
			#endif
			}
		}
		deferredEnd(success);
	}
	
	return success;
}

void SceneEditor::drawNode(const SceneNode & node) const
{
	const bool isHovered = node.id == hoverNodeId;
	const bool isSelected = selection.selectedNodes.count(node.id) != 0;
	
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
			
			setAlpha(255);
			
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

void SceneEditor::drawEditorGizmosOpaque(const bool depthObscuredPass) const
{
#if ENABLE_TRANSFORM_GIZMOS
	transformGizmo.drawOpaque(depthObscuredPass);
#endif
}

void SceneEditor::drawEditorGizmosTranslucent() const
{
#if ENABLE_TRANSFORM_GIZMOS
	transformGizmo.drawTranslucent();
#endif
}

void SceneEditor::drawEditorSelectedNodeLabels() const
{
	int viewportX;
	int viewportY;
	int viewportSx;
	int viewportSy;
	getEditorViewport(viewportX, viewportY, viewportSx, viewportSy);
	
	Mat4x4 projectionMatrix;
	camera.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);
	projectionMatrix = Mat4x4(true)
		.Translate(viewportX, viewportY, 0)
		.Mul(projectionMatrix);

	Mat4x4 viewMatrix;
	camera.calculateViewMatrix(viewMatrix);
	
	const Mat4x4 modelViewProjection = projectionMatrix * viewMatrix;
	
	for (auto nodeId : selection.selectedNodes)
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

void SceneEditor::drawGui() const
{
	const_cast<SceneEditor*>(this)->guiContext.draw();
}

void SceneEditor::drawView() const
{
	drawEditorSelectedNodeLabels();
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
			deferredEnd(true);
			
			scene = tempScene;
			tempScene.nodes.clear();
			
			undoReset();
			
			return true;
		}
	}
}

void SceneEditor::resetDocument()
{
	selection = Selection();
	
	deferred = Deferred();
	
	nodeUi = NodeUi();
	
	parameterUi = ParameterUi();
	
	undo = Undo();
	
	documentInfo = DocumentInfo();
}

void SceneEditor::updateFilePaths(Scene & scene, TypeDB * typeDB, const char * oldBasePath, const char * newBasePath)
{
	// when we change document paths, all relative paths should be updated to reflect the new 'base path' (the path where the scene is saved)
	
	// we will also convert all absolute paths into relative paths, as the scene may already be edited and contain some absolute paths when the document path wasn't known at the time of making these edits
	
	// fetch all file paths inside the scene
	
	std::vector<std::string*> filePaths;
	
	for (auto & node_itr : scene.nodes)
	{
		auto * node = node_itr.second;
		
		for (auto * component = node->components.head; component != nullptr; component = component->next_in_set)
		{
			auto * componentType = findComponentType(component->typeIndex());
			
			for (auto * member = componentType->members_head; member != nullptr; member = member->next)
			{
				const bool isFilePath = member->hasFlag<ComponentMemberFlag_EditorType_FilePath>();
				
				if (isFilePath == false)
					continue;
					
				if (member->isVector)
				{
					auto * member_vector = static_cast<const Member_VectorInterface*>(member);
					
					auto * vector_type = typeDB->findType(member_vector->vector_type());
					
					Assert(vector_type->isStructured == false); // the FilePath flag shouldn't be set on structured types
					if (vector_type->isStructured == false)
					{
						auto * plain_type = static_cast<const PlainType*>(vector_type);
						
						for (int i = 0; i < member_vector->vector_size(component); ++i)
						{
							auto * vector_object = member_vector->vector_access(component, i);
							
							if (plain_type->dataType == kDataType_String)
							{
								std::string * filePath = &plain_type->access<std::string>(vector_object);
								
								filePaths.push_back(filePath);
							}
						}
					}
				}
				else
				{
					auto * member_scalar = static_cast<const Member_Scalar*>(member);
					
					auto * member_type = typeDB->findType(member_scalar->typeIndex);
					auto * member_object = member_scalar->scalar_access(component);
					
					Assert(member_type->isStructured == false); // the FilePath flag shouldn't be set on structured types
					if (member_type->isStructured == false)
					{
						auto * plain_type = static_cast<const PlainType*>(member_type);
						
						if (plain_type->dataType == kDataType_String)
						{
							std::string * filePath = &plain_type->access<std::string>(member_object);
							
							filePaths.push_back(filePath);
						}
					}
				}
			}
		}
	}
	
	// convert all paths into absolute paths
	
	auto isAbsolutePath = [](const char * path)
	{
		return
			path[0] == '/' ||
			strchr(path, ':') != nullptr;
	};
	
	if (oldBasePath[0] != 0)
	{
		for (auto * filePath : filePaths)
		{
			// don't attempt to change empty paths
			if (filePath->empty())
				continue;
				
			// don't attempt to make absolute paths absolute
			if (isAbsolutePath(filePath->c_str()))
				continue;
			
			// make the path absolute
			*filePath = Path::MakeAbsolute(oldBasePath, *filePath);
		}
	}
				
	// convert all paths into relative paths, given the new document path
	
	if (newBasePath[0] != 0)
	{
		for (auto * filePath : filePaths)
		{
			// don't attempt to change empty paths
			if (filePath->empty())
				continue;
				
			*filePath = Path::MakeRelative(newBasePath, *filePath);
		}
	}
}
