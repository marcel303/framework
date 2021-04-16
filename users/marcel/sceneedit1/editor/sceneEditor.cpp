// ecs-sceneEditor
#include "editor/componentPropertyUi.h"
#include "editor/parameterComponentUi.h"
#include "editor/raycast.h"
#include "editor/scene_clipboardHelpers.h"
#include "editor/sceneEditor.h"
#include "editor/transformGizmos.h"

// ecs-scene
#define DEFINE_COMPONENT_TYPES
#include "components/gltfComponent.h"
#include "components/modelComponent.h"
#include "components/parameterComponent.h"
#include "components/transformComponent.h"
#include "helpers.h" // findComponentType
#include "scene.h"
#include "sceneIo.h"
#include "sceneNodeComponent.h"
#include "templateIo.h"

// ecs-component
#include "componentType.h"
#include "componentTypeDB.h"

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
#include "Calc.h"
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

// filedialog
#if SCENEEDIT_USE_LIBNFD
	#include "nfd.h"
#endif
#if SCENEEDIT_USE_IMGUIFILEDIALOG
	#include "ImGuiFileDialog.h"
#endif


//

#if defined(DEBUG)
	#define DEBUG_UNDO 0
#else
	#define DEBUG_UNDO 0 // do not alter
#endif

#define ENABLE_TRANSFORM_RESTORE_ON_SCENE_UPDATE 1

#define ENABLE_NODE_SELECTION_RESTORE_ON_UNDO_REDO 0 // todo : requires unique names for each nodes to always exist (not just on auto assign during save)

static const float kMinNodeRadius = .5f;

#if SCENEEDIT_USE_IMGUIFILEDIALOG
static void openImGuiOpenDialog(const char * key, const char * caption, const char * filter, const char * initialPath);
static void openImGuiSaveDialog(const char * key, const char * caption, const char * filter, const char * initialPath);
static const char * doImGuiFileDialog(const SceneEditor & sceneEditor, const char * key);
#endif

//

inline bool modShift()
{
	return
		keyboard.isDown(SDLK_LSHIFT) ||
		keyboard.isDown(SDLK_RSHIFT);
}

inline bool keyCommand()
{
#if defined(MACOS) || defined(IPHONEOS)
	return
		keyboard.isDown(SDLK_LGUI) ||
		keyboard.isDown(SDLK_RGUI);
#else
	return
		keyboard.isDown(SDLK_LCTRL) ||
		keyboard.isDown(SDLK_RCTRL);
#endif
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
	
	initTemplateUi();
	
	//
	
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

static bool getBoundingBoxForNode(const SceneNode & node, Vec3 & min, Vec3 & max)
{
	bool hasMinMax = false;
	
	for (auto * component = node.components.head; component != nullptr; component = component->next_in_set)
	{
		if (hasMinMax)
		{
			Vec3 compMin(false);
			Vec3 compMax(false);
			
			if (component->getBoundingBox(compMin, compMax))
			{
				min = min.Min(compMin);
				max = max.Max(compMax);
			}
		}
		else
		{
			hasMinMax = component->getBoundingBox(min, max);
		}
	}
	
	return hasMinMax;
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
		
		// intersect the accurate AABB for the node (if it has one)
		
		Vec3 min(false);
		Vec3 max(false);
		if (getBoundingBoxForNode(node, min, max))
		{
			const auto & objectToWorld = sceneNodeComp->objectToWorld;
			const auto worldToObject = objectToWorld.CalcInv();
			
			const Vec3 rayOrigin_object = worldToObject.Mul4(rayOrigin);
			const Vec3 rayDirection_object = worldToObject.Mul3(rayDirection);
			
			float distance;
			if (intersectBoundingBox3d(
				&min[0],
				&max[0],
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
		
		// intersect a sphere in world-space, which ensures the node always has a minimum size representation within the world to click on
		
		const Vec3 position = sceneNodeComp->objectToWorld.GetTranslation();
		float sphere_t1;
		float sphere_t2;
		if (intersectSphere(
			rayOrigin[0],
			rayOrigin[1],
			rayOrigin[2],
			rayDirection[0],
			rayDirection[1],
			rayDirection[2],
			position[0],
			position[1],
			position[2],
			kMinNodeRadius,
			sphere_t1,
			sphere_t2))
		{
			const float distance = fminf(fmaxf(0.f, sphere_t1), sphere_t2);
			
			if (distance < bestDistance)
			{
				bestNode = &node;
				bestDistance = distance;
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
				
				parentNodesToParent();
				
				selectNodesToSelect(false);
				
				deferred.isProcessing = false;
			}
			undoCaptureEnd();
			
			Assert(deferred.isFlushed());
			
			// one or more transforms may need to be updated, due to new nodes being added or due to parenting operations changing the tree structure
			g_transformComponentMgr.calculateTransforms(scene);
		}
		else
		{
			auto & stackItem = deferred.stack.back();
			
			for (auto * node : deferred.nodesToAdd)
				stackItem.nodesToAdd.push_back(node);
			deferred.nodesToAdd.clear();
			
			for (auto nodeId : deferred.nodesToRemove)
				stackItem.nodesToRemove.insert(nodeId);
			deferred.nodesToRemove.clear();
			
			for (auto parent : deferred.nodesToParent)
				stackItem.nodesToParent.insert(parent);
			deferred.nodesToParent.clear();
			
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
		deferred.nodesToParent.clear();
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
		
		if (nodeId == hoverNodeId)
			hoverNodeId = -1;
		
		deferred.nodesToRemove.erase(nodeId);
		deferred.nodesToSelect.erase(nodeId);
		deferred.nodesToParent.erase(nodeId);
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

void SceneEditor::parentNodesToParent()
{
	AssertMsg(deferred.isProcessing, "this method may only be called by deferredEnd!");
	
	for (auto & nodeToParent : deferred.nodesToParent)
	{
		auto & childNode = scene.getNode(nodeToParent.first);
		auto & oldParent = scene.getNode(childNode.parentId);
		auto & newParent = scene.getNode(nodeToParent.second);
		
		oldParent.childNodeIds.erase(
			std::find(
				oldParent.childNodeIds.begin(),
				oldParent.childNodeIds.end(),
				childNode.id));
				
		newParent.childNodeIds.push_back(childNode.id);
		childNode.parentId = newParent.id;
	}
	
	deferred.nodesToParent.clear();
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

void SceneEditor::undoCaptureBegin(const bool isPristine)
{
	Assert(deferred.numActivations == 0);
	
#if ENABLE_NODE_SELECTION_RESTORE_ON_UNDO_REDO
	// note : we update the selected nodes during undoCaptureBegin, because
	//        we always be as up to date as possible up to the point some
	//        edits are made
	
	undo.versions[undo.currentVersionIndex].selectedNodes = selection.selectedNodes;
#endif
	
#if defined(DEBUG)
	/**
	 * For debugging purposes, we save the current version, and compare it later when we begin
	 * capturing a new undo state. these *should* be equal, unless some action didn't get recorded into
	 * the undo history.
	 */
	
	if (isPristine)
	{
		std::string text;
		
		if (undoCapture(text))
		{
			if (text != undo.currentVersion)
			{
				printf("got:\n%s\n", text.c_str());
				printf("expected:\n%s\n", undo.currentVersion.c_str());
			}
			Assert(text == undo.currentVersion);
		}
	}
#endif
}

void SceneEditor::undoCaptureEnd()
{
	std::string text;
	
	if (undoCapture(text))
	{
		// note : resize in case the current capture will obliterate some
		//        older captures, due to undo going back a few versions
		undo.versions.resize(undo.currentVersionIndex + 1);
	#if ENABLE_NODE_SELECTION_RESTORE_ON_UNDO_REDO
		undo.versions.emplace_back(UndoVersion { text, selection.selectedNodes });
	#else
		undo.versions.emplace_back(UndoVersion { text, { } });
	#endif
		undo.currentVersionIndex = undo.versions.size() - 1;
		
	#if defined(DEBUG)
		undo.currentVersion = text;
	#endif
	}
}

void SceneEditor::undoCaptureFastForward()
{
#if defined(DEBUG)
	std::string text;
	
	if (undoCapture(text))
	{
		undo.currentVersion = text;
	}
#endif
}

void SceneEditor::undoReset()
{
	undo.versions.clear();
	undo.currentVersion.clear();
	undo.currentVersionIndex = -1;
	
	undoCapture(undo.currentVersion);
#if ENABLE_NODE_SELECTION_RESTORE_ON_UNDO_REDO
	undo.versions.push_back({ undo.currentVersion, selection.selectedNodes });
#else
	undo.versions.push_back({ undo.currentVersion, { } });
#endif
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
				const auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
				
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
					const auto * first_type = g_componentTypeDB.findComponentType(first->typeIndex());
					const auto * second_type = g_componentTypeDB.findComponentType(second->typeIndex());
					
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
					const auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
					
					Assert(componentType != nullptr);
					if (componentType != nullptr)
					{
						bool enabled = component->enabled;
						if (ImGui::Checkbox("", &enabled))
						{
							undoCaptureBegin();
							{
								component->enabled = enabled;
							}
							undoCaptureEnd();
						}
						ImGui::SameLine();

						const bool wasOpen = (component->editorFlags & kComponentEditorFlag_Folded) == 0;

						const bool isOpen = ImGui::TreeNodeEx(&node,
							(ImGuiTreeNodeFlags_OpenOnArrow * 0) |
							(ImGuiTreeNodeFlags_DefaultOpen * wasOpen) |
							(ImGuiTreeNodeFlags_FramePadding * 0),
							"%s", componentType->typeName);

						if (isOpen != wasOpen)
						{
							undoCaptureBegin();
							{
								if (isOpen)
									component->editorFlags &= ~kComponentEditorFlag_Folded;
								else
									component->editorFlags |= kComponentEditorFlag_Folded;
							}
							undoCaptureEnd();
						}

						if (isOpen)
						{
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
							callbacks.propertyWillChange = [this]()
								{
									undoCaptureBegin(false);
								};
							callbacks.propertyDidChange = [this]()
								{
									undoCaptureEnd();
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

							ImGui::TreePop();
						}
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
			auto * sceneNodeComponent = g_sceneNodeComponentMgr.createComponent(childNode->components.id);
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
			deferredEnd(true);
			
			return true;
		}
	}
}

bool SceneEditor::pasteNodeTreeFromLines(const int parentId, LineReader & line_reader, const bool keepRootNode)
{
	Scene childScene;
	childScene.nodeIdAllocator = &scene;
	
	// create root node, and make sure it has a transform component
	childScene.createRootNode();
	
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
			auto * sceneNodeComponent = g_sceneNodeComponentMgr.createComponent(node.components.id);
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
				
				if (keepRootNode)
				{
					// re-parent the node if it is the root node
					
					if (node->id == childScene.rootNodeId)
					{
						node->parentId = parentId;
						deferred.nodesToSelect.insert(node->id);
					}
					
					deferred.nodesToAdd.push_back(node);
				}
				else
				{
					// skip this node if its the root node
					
					if (node->id == childScene.rootNodeId)
					{
						node->freeComponents();
						
						delete node;
						node = nullptr;
						
						continue;
					}
					
					// re-parent the node if it was parented to the root node
					
					if (node->parentId == childScene.rootNodeId)
					{
						node->parentId = parentId;
						deferred.nodesToSelect.insert(node->id);
					}
					
					deferred.nodesToAdd.push_back(node);
				}
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
	// todo : only add undo capture on completion of editing operation..
		undoCaptureBegin();
		{
			sceneNodeComponent->name = name;
		}
		undoCaptureEnd();
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
	
	ImGui::Separator();
	
	if (ImGui::MenuItem("Duplicate", nullptr, false, node.parentId != -1))
	{
		result = kNodeStructureEditingAction_NodeDuplicate;
	}
	
	if (ImGui::MenuItem("Remove", nullptr, false, node.parentId != -1))
	{
		result = kNodeStructureEditingAction_NodeRemove;
	}

// todo : add some add child node versions with content :
// + mesh : cube
// + mesh : cylinder
// - mesh : plane
// + mesh : quad
// + light point
// + light directional
// + light spot
// - light area box
// - audio emitter (with source) (how is source referenced? resource pointer.. by name.. ??)
// - video
// todo : add vfx graph nodes for : video by name
// todo : vfx graph component : addObject<VfxNodeScene> .. or something .. that gives access to the scene .. allow access to scene nodes by name .. useful for video node for instance, to start / stop playback of a specific video hosted by a node
// todo : automatically (?) add vfx node types for all registered component types and their inputs
	if (ImGui::MenuItem("Add child node"))
	{
		result = kNodeStructureEditingAction_NodeAddChild;
	}
	
	if (ImGui::BeginMenu("Add child node.."))
	{
		std::function<void(const TemplateUi::TemplateElem & elem)> doElem;
		
		doElem = [&](const TemplateUi::TemplateElem & elem)
		{
			for (auto & childElem : elem.children)
			{
				if (childElem.path.empty())
				{
					if (ImGui::BeginMenu(childElem.name.c_str()))
					{
						doElem(childElem);
						ImGui::EndMenu();
					}
				}
				else
				{
					if (ImGui::MenuItem(childElem.name.c_str()))
					{
						result = kNodeStructureEditingAction_NodeAddChildFromTemplate;
						action_nodeAddFromTemplate.path = framework.resolveResourcePath(childElem.path.c_str());
					}
				}
			}
		};
		
		doElem(templateUi.rootTemplateElem);
		
		ImGui::EndMenu();
	}
	
	ImGui::Separator();
	
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
	
	ImGui::Separator();
	
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
			auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
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
		
		auto * componentType = g_componentTypeDB.findComponentType(clipboardInfo.componentTypeName.c_str());
		
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
			auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
			
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
		for (auto * componentType : g_componentTypeDB.componentTypes)
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
			// note : we preserve the selection when additive mode (shift) is used or when the context menu is summoned
			if (!ImGui::GetIO().KeyShift && !ImGui::IsItemClicked(1))
				selection = Selection();
			selection.selectedNodes.insert(node.id);
		}
		
		if (ImGui::BeginDragDropTarget())
		{
			const ImGuiPayload * payload = ImGui::AcceptDragDropPayload("SceneNodeId");
			
			if (payload != nullptr)
			{
				const int childNodeId = *(int*)payload->Data;
				
				// the new parent should not as one of its ancesters have the child node
				// otherwise, a graph cycle will occur. check if there would be a cycle
				// and discard the drag and drop operation if so
				
				bool generatesCycle = false;
				int nextNodeId = node.id;
				
				for (;;)
				{
					auto & nextNode = scene.getNode(nextNodeId);
					if (nextNode.parentId == -1)
						break;
					generatesCycle |= nextNode.parentId == childNodeId;
					nextNodeId = nextNode.parentId;
				}
				
				if (generatesCycle == false)
				{
					result = kNodeStructureEditingAction_NodeParent;
					action_nodeParent.childNodeId = childNodeId;
					action_nodeParent.newParentNodeId = node.id;
				}
			}
			ImGui::EndDragDropTarget();
		}
		
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceNoPreviewTooltip))
		{
			ImGui::SetDragDropPayload("SceneNodeId", &nodeId, sizeof(nodeId));
			ImGui::EndDragDropSource();
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
					auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
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

int SceneEditor::addNodesFromScene(const char * path, const int parentId)
{
	int rootNodeId;
	if (addNodesFromScene(path, parentId, true, &rootNodeId))
		return rootNodeId;
	else
		return -1;
}

bool SceneEditor::addNodesFromScene(const char * path, const int parentId, const bool keepRootNode, int * out_rootNodeId)
{
	Scene childScene;
	childScene.nodeIdAllocator = &scene;
	
	// 1. create root node
	
	childScene.createRootNode();
	auto & rootNode = childScene.getRootNode();
	
	if (keepRootNode)
	{
		// 2.1 assign its parent id
		
		rootNode.parentId = parentId;
		
		if (out_rootNodeId != nullptr)
			*out_rootNodeId = rootNode.id;
	}
	
	// 2.2 assign its display name
	
	auto * sceneNodeComponent = g_sceneNodeComponentMgr.getComponent(rootNode.components.id);
	
	sceneNodeComponent->name = Path::GetBaseName(path);
	
	// 3. and make sure it has a transform component
	
	auto * transformComponent = g_transformComponentMgr.createComponent(rootNode.components.id);
	rootNode.components.add(transformComponent);
	
	// 4. load scene from file
	
	if (!parseSceneFromFile(*typeDB, path, childScene))
	{
		childScene.freeAllNodesAndComponents();
		return false;
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
		return false;
	}
	else
	{
		auto & rootNode = childScene.getRootNode();
		
		// if all went well, move the nodes to the parent scene
		
		deferredBegin();
		{
			if (keepRootNode)
			{
				for (auto node_itr : childScene.nodes)
				{
					deferred.nodesToAdd.push_back(node_itr.second);
				}
				
				deferred.nodesToSelect.insert(childScene.rootNodeId);
			}
			else
			{
				for (auto node_itr : childScene.nodes)
				{
					auto * node = node_itr.second;
					
					if (node->id == childScene.rootNodeId)
					{
						// delete the node if it's the root node
						
						node->freeComponents();
						
						delete node;
						node = nullptr;
					}
					else
					{
						// if not the root node, keep it and add it to the scene
						
						deferred.nodesToAdd.push_back(node);
						
						if (node->parentId == childScene.rootNodeId)
						{
							node->parentId = parentId;
							
							deferred.nodesToSelect.insert(node->id);
						}
					}
				}
			}
			
			childScene.nodes.clear();
		}
		deferredEnd(true);
		
		return true;
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
			SceneNode * rootNode = deferred.getNodeToAdd(rootNodeId);
			Assert(rootNode != nullptr);
				
			auto * sceneNodeComponent = g_sceneNodeComponentMgr.getComponent(rootNode->components.id);
		
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
			SceneNode * newRootNode = deferred.getNodeToAdd(newRootNodeId);
			Assert(newRootNode != nullptr);
			
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
	
	tickKeyCommands(inputIsCaptured);
	
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
				if (editingMode == kEditingMode_NodePlacement)
				{
					if (ImGui::CollapsingHeader("Node Placement", ImGuiTreeNodeFlags_DefaultOpen))
					{
						char templatePath[256];
						strcpy_s(templatePath, sizeof(templatePath), nodePlacement.templatePath.c_str());
						if (ImGui::InputText("Template", templatePath, sizeof(templatePath)))
							nodePlacement.templatePath = templatePath;
							
					#if SCENEEDIT_USE_LIBNFD
						ImGui::SameLine();
						if (ImGui::Button(".."))
						{
							nfdchar_t * path = nullptr;
							
							if (NFD_OpenDialog(nullptr, nullptr, &path) == NFD_OKAY)
							{
								nodePlacement.templatePath = makePathRelativeToDocumentPath(path);
							}
						
							if (path != nullptr)
							{
								free(path);
								path = nullptr;
							}
						}
					#elif SCENEEDIT_USE_IMGUIFILEDIALOG
						ImGui::SameLine();
						const char * dialogKey = "PlacementTemplatePath";
						if (ImGui::Button(".."))
						{
							openImGuiOpenDialog(
								dialogKey,
								"Select file..",
								".*",
								nodePlacement.templatePath.c_str());
						}
						
						const char * path = doImGuiFileDialog(*this, dialogKey);
						
						if (path != nullptr)
						{
							nodePlacement.templatePath = makePathRelativeToDocumentPath(path);
						}
					#endif
					}
				}
				
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
						ImGui::SliderFloat("", &preview.tickMultiplier, 0.f, 100.f, "%.2f", ImGuiSliderFlags_Logarithmic);
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
				
				// scene structure
				
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
					#if SCENEEDIT_USE_IMGUIFILEDIALOG
						const char * attachSceneDialogKey = "AttachScene";
						const char * importSceneDialogKey = "ImportScene";
					#endif
					
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
						case kNodeStructureEditingAction_NodeDuplicate:
							performAction_duplicate();
							break;
							
						case kNodeStructureEditingAction_NodeRemove:
							performAction_remove();
							break;
							
						case kNodeStructureEditingAction_NodeAddChild:
							performAction_addChild();
							break;
						
						case kNodeStructureEditingAction_NodeAddChildFromTemplate:
							performAction_sceneImport(action_nodeAddFromTemplate.path.c_str());
							action_nodeAddFromTemplate = Action_NodeAddFromTemplate();
							break;
							
						case kNodeStructureEditingAction_NodeSceneAttach:
						#if SCENEEDIT_USE_LIBNFD
							performAction_sceneAttach(nullptr, nullptr);
						#elif SCENEEDIT_USE_IMGUIFILEDIALOG
							openImGuiOpenDialog(
								attachSceneDialogKey,
								"Attach Scene..",
								".*",
								"");
						#endif
							break;
						case kNodeStructureEditingAction_NodeSceneAttachUpdate:
							performAction_sceneAttachUpdate();
							break;
							
						case kNodeStructureEditingAction_NodeSceneImport:
						#if SCENEEDIT_USE_LIBNFD
							performAction_sceneImport(nullptr);
						#elif SCENEEDIT_USE_IMGUIFILEDIALOG
							openImGuiOpenDialog(
								importSceneDialogKey,
								"Import Scene..",
								".*",
								"");
						#endif
							break;
							
						case kNodeStructureEditingAction_NodeParent:
							performAction_parent(
								action_nodeParent.childNodeId,
								action_nodeParent.newParentNodeId);
							action_nodeParent = Action_NodeParent();
							break;
						}
						
					#if SCENEEDIT_USE_IMGUIFILEDIALOG
						const char * path;
					
						if ((path = doImGuiFileDialog(*this, attachSceneDialogKey)))
						{
							performAction_sceneAttach(makePathRelativeToDocumentPath(path).c_str(), nullptr);
						}
						
						if ((path = doImGuiFileDialog(*this, importSceneDialogKey)))
						{
							performAction_sceneImport(makePathRelativeToDocumentPath(path).c_str());
						}
					#endif
					}
					ImGui::EndChild();
					
					if (ImGui::IsItemHovered())
					{
						if (ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Delete)) ||
							ImGui::IsKeyPressed(ImGui::GetKeyIndex(ImGuiKey_Backspace)))
						{
							performAction_remove();
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
					g_transformComponentMgr.calculateTransforms(scene);
				}
			}
			ImGui::End();
		}
		
		// menu bar
		
		if (ImGui::BeginMainMenuBar())
		{
		#if SCENEEDIT_USE_IMGUIFILEDIALOG
			const char * loadSceneDialogKey = "LoadScene";
			const char * saveSceneDialogKey = "SaveScene";
		#endif
				
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("Load"))
				{
				#if SCENEEDIT_USE_LIBNFD
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
				#elif SCENEEDIT_USE_IMGUIFILEDIALOG
					openImGuiOpenDialog(
						loadSceneDialogKey,
						"Load Scene..",
						".*",
						"");
				#endif
				}
				
				if (ImGui::MenuItem("Reload"))
				{
					if (!documentInfo.path.empty())
					{
						const std::string path = documentInfo.path;
						
						performAction_load(path.c_str());
					}
				}
				
				if (ImGui::MenuItem("Save", nullptr, false, !documentInfo.path.empty()))
				{
					performAction_save();
				}
				
				if (ImGui::MenuItem("Save as.."))
				{
				#if SCENEEDIT_USE_LIBNFD
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
				#elif SCENEEDIT_USE_IMGUIFILEDIALOG
					openImGuiSaveDialog(
						saveSceneDialogKey,
						"Save Scene..",
						".*",
						"");
				#endif
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
			
			if (ImGui::BeginMenu("Parameters"))
			{
				ImGui::InputText("Component filter", parameterUi.component_filter, kMaxParameterFilter);
				ImGui::InputText("Parameter filter", parameterUi.parameter_filter, kMaxParameterFilter);
				ImGui::Checkbox("Show components with empty prefix", &parameterUi.showAnonymousComponents);
				
				doParameterUi(g_parameterComponentMgr, parameterUi.component_filter, parameterUi.parameter_filter, parameterUi.showAnonymousComponents);
				
				ImGui::EndMenu();
			}
			
			if (ImGui::BeginMenu("Editing Mode"))
			{
				if (ImGui::MenuItem("Node Selection"))
					editingMode = kEditingMode_NodeSelection;
				if (ImGui::MenuItem("Node Placement"))
					editingMode = kEditingMode_NodePlacement;
					
				ImGui::EndMenu();
			}
			
			//
			
		#if SCENEEDIT_USE_IMGUIFILEDIALOG
			const char * path;
			
			if ((path = doImGuiFileDialog(*this, loadSceneDialogKey)))
			{
				performAction_load(path);
			}
			
			if ((path = doImGuiFileDialog(*this, saveSceneDialogKey)))
			{
				performAction_save(path, true);
			}
		#endif
		}
		ImGui::EndMainMenuBar();
	
		guiContext.updateMouseCursor();
	}
	guiContext.processEnd();
}

void SceneEditor::tickView(const float dt, bool & inputIsCaptured)
{
	tickKeyCommands(inputIsCaptured);
	
	Mat4x4 worldToView;
	if (framework.isStereoVr())
		gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
	else
		camera.calculateViewMatrix(worldToView);
	
	const Mat4x4 viewToWorld = worldToView.CalcInv();
	
	// -- mouse picking
	
	// 1. transform mouse coordinates into a world space direction vector
	
	Vec3 pointerOrigin_world;
	Vec3 pointerDirection_world;
	bool hasPointer = false;
	bool pointerIsActive = false;
	bool pointerBecameActive = false;
	bool secondaryIsActive = false;
	bool secondaryBecameActive = false;
	
	if (framework.isStereoVr())
	{
		if (vrPointer[0].hasTransform)
		{
			const Mat4x4 transform = vrPointer[0].getTransform(framework.vrOrigin);
			pointerOrigin_world = transform.GetTranslation();
			pointerDirection_world = transform.GetAxis(2);
			hasPointer = true;
		}
		
		pointerIsActive = vrPointer[0].isDown(VrButton_Trigger);
		pointerBecameActive = vrPointer[0].wentDown(VrButton_Trigger);
		
		secondaryIsActive = vrPointer[0].isDown(VrButton_GripTrigger);
		secondaryBecameActive = vrPointer[0].wentDown(VrButton_GripTrigger);
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
		
		if (camera.mode == Camera::kMode_Ortho)
		{
			const Vec2 mousePosition_screen(
				mouse.x,
				mouse.y);
			const Vec3 mousePosition_clip(
				(mousePosition_screen[0] - viewportX) / float(viewportSx) * 2.f - 1.f,
				(mousePosition_screen[1] - viewportY) / float(viewportSy) * 2.f - 1.f,
				0.f);
			Vec3 mousePosition_view = projectionMatrix.CalcInv().Mul4(mousePosition_clip);
			mousePosition_view[1] = -mousePosition_view[1];
			
			pointerOrigin_world = viewToWorld.Mul4(mousePosition_view) - viewToWorld.GetAxis(2) * 10.f;
			
			pointerDirection_world = viewToWorld.GetAxis(2);
			pointerDirection_world = pointerDirection_world.CalcNormalized();
		}
		else
		{
			pointerOrigin_world = viewToWorld.GetTranslation();
			
			const Vec2 mousePosition_screen(
				mouse.x,
				mouse.y);
			const Vec2 mousePosition_clip(
				(mousePosition_screen[0] - viewportX) / float(viewportSx) * 2.f - 1.f,
				(mousePosition_screen[1] - viewportY) / float(viewportSy) * 2.f - 1.f);
			Vec2 mousePosition_view = projectionMatrix.CalcInv().Mul4(mousePosition_clip);
			
			pointerDirection_world = viewToWorld.Mul3(
				Vec3(
					+mousePosition_view[0],
					-mousePosition_view[1],
					1.f));
			pointerDirection_world = pointerDirection_world.CalcNormalized();
		}
		
		hasPointer = true;
		
		pointerIsActive = mouse.isDown(BUTTON_LEFT) && !keyCommand();
		pointerBecameActive = mouse.wentDown(BUTTON_LEFT) && !keyCommand();
		
		secondaryIsActive = mouse.isDown(BUTTON_LEFT) && keyCommand();
		secondaryBecameActive = mouse.wentDown(BUTTON_LEFT) && keyCommand();
	}
	
#if ENABLE_TRANSFORM_GIZMOS
	// 2.1 transformation gizmo interaction
	
	transformGizmo.editingWillBegin = [this]()
		{
			undoCaptureBegin();
		};
	transformGizmo.editingDidEnd = [this]()
		{
			undoCaptureEnd();
		};
		
	if (selection.selectedNodes.size() != 1)
	{
		transformGizmo.hide();
	}
	else
	{
		auto & node = scene.getNode(*selection.selectedNodes.begin());
		
		auto * sceneNodeComponent = node.components.find<SceneNodeComponent>();
		auto * transformComponent = node.components.find<TransformComponent>();
		
		if (sceneNodeComponent == nullptr || transformComponent == nullptr)
		{
			transformGizmo.hide();
		}
		else
		{
			// determine the current global transform
			
			const Mat4x4 & globalTransform = sceneNodeComponent->objectToWorld;
		
			// let the gizmo do it's thing
			
			transformGizmo.show(globalTransform);
			
			if (transformGizmo.tick(
					pointerOrigin_world,
					pointerDirection_world,
					pointerIsActive && hasPointer,
					pointerBecameActive && hasPointer, // todo : propagate hasPointer to transform gizmo (?)
					inputIsCaptured,
					viewToWorld.GetTranslation(),
					viewToWorld.GetAxis(2).CalcNormalized()))
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
				
				// update transforms after editing a transform using the transform gizmo
				// note : we could optimize this by only calculating transforms for the
				//        sub-tree starting at the node being edited by the gizmo. however,
				//        this isn't so trivial to do at the moment
				g_transformComponentMgr.calculateTransforms(scene);
			}
		}
	}
#endif

	// 2.1 interactive ring interaction
	
	const bool ringIsActive = secondaryIsActive && hasPointer;
	
	if (ringIsActive && !interactiveRing.captureElem.hasCapture)
	{
		inputIsCaptured = true;
		
		const float distance =
			framework.isStereoVr()
			? 2.f
			: 5.f;
			
		const Mat4x4 transform =
			Mat4x4(true)
			.Translate(viewToWorld.GetTranslation())
			.Translate(viewToWorld.GetAxis(2) * distance);
		
		interactiveRing.show(transform);
	}
	
	interactiveRing.tick(
		*this,
		pointerOrigin_world,
		pointerDirection_world,
		viewToWorld,
		ringIsActive,
		inputIsCaptured,
		dt);

	// 3. determine which node is underneath the mouse cursor
	
	{
		const SceneNode * hoverNode =
			inputIsCaptured == false && hasPointer
			? raycast(pointerOrigin_world, pointerDirection_world, selection.selectedNodes)
			: nullptr;
		
		hoverNodeId = hoverNode ? hoverNode->id : -1;
	}
	
	// 4. update mouse cursor

// todo : framework should mediate here. perhaps have a requested mouse cursor and update it each frame. or make it a member of the mouse, and add an explicit mouse.updateCursor() method

	if (inputIsCaptured == false)
	{
	#if FRAMEWORK_USE_SDL
		static SDL_Cursor * cursorHand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		SDL_SetCursor(hoverNodeId == -1 ? SDL_GetDefaultCursor() : cursorHand);
	#endif
	}
	
	if (editingMode == kEditingMode_NodeSelection)
	{
		// action: select node(s)
		if (inputIsCaptured == false && hasPointer && pointerBecameActive)
		{
			inputIsCaptured = true;
			
			if (!modShift())
				selection = Selection();
			
			if (hoverNodeId != -1)
			{
				selection.selectedNodes.insert(hoverNodeId);
				markNodeOpenUntilRoot(hoverNodeId);
				nodeUi.nodeToGiveFocus = hoverNodeId;
			}
		}
	}
	
	if (editingMode == kEditingMode_NodePlacement)
	{
		// action: duplicate node at raycast position
		if (inputIsCaptured == false && hasPointer && pointerBecameActive)
		{
			inputIsCaptured = true;
		
			// find intersection point with the ground plane
		
			if (pointerDirection_world[1] != 0.f)
			{
				const float t = -pointerOrigin_world[1] / pointerDirection_world[1];
				
				if (t >= 0.f)
				{
					const Vec3 groundPosition = pointerOrigin_world + pointerDirection_world * t;
					
					bool success = true;

					deferredBegin();
					{
						std::vector<int> rootNodeIds;
						
						const int nodeId = attachScene(
							nodePlacement.templatePath.c_str(),
							scene.rootNodeId);
						
						success &= nodeId != -1;
						
						if (nodeId != -1)
						{
							auto * node = deferred.getNodeToAdd(nodeId);
							
							auto parentNodeId = node->parentId;
							
							auto & parentNode = scene.getNode(parentNodeId);
							
							auto * sceneNodeComp = parentNode.components.find<SceneNodeComponent>();
							
							auto * transformComp = node->components.find<TransformComponent>();
							
							Assert(sceneNodeComp != nullptr && transformComp != nullptr);
							if (sceneNodeComp != nullptr && transformComp != nullptr)
							{
								const Vec3 groundPosition_parent = sceneNodeComp->objectToWorld.CalcInv().Mul4(groundPosition);
								
								transformComp->position = groundPosition_parent;
							}
						}
					}
					deferredEnd(success);
				}
			}
		}
	}
	
	// action: remove selected node(s)
	if (inputIsCaptured == false && (keyboard.wentDown(SDLK_BACKSPACE) || keyboard.wentDown(SDLK_DELETE)))
	{
		inputIsCaptured = true;
		
		performAction_remove();
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

void SceneEditor::tickKeyCommands(bool & inputIsCaptured)
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
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_p) && keyCommand())
	{
		inputIsCaptured = true;
		preview.tickMultiplier *= 1.25f;
	}
	
	// action: decrease simulation speed
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_o) && keyCommand())
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
		if (modShift())
			performAction_pasteChild();
		else
			performAction_pasteSibling();
	}
	
	// action: duplicate selected node(s)
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_d) && keyCommand())
	{
		inputIsCaptured = true;
		performAction_duplicate();
	}
	
	// action: undo
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_z) && keyCommand() && !modShift())
	{
		inputIsCaptured = true;
		performAction_undo();
	}
	
	// action: redo
	if (inputIsCaptured == false && keyboard.wentDown(SDLK_z) && keyCommand() && modShift())
	{
		inputIsCaptured = true;
		performAction_redo();
	}
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
	#if SCENEEDIT_USE_LIBNFD
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
	#endif
	}
	else
	{
		performAction_save(documentInfo.path.c_str(), false);
	}
}

void SceneEditor::performAction_save(const char * path, const bool updateDocumentPath)
{
	// note : saving the scene may auto-generate some names, and update the
	//        file paths. so we need to begin an undo capture here and fast forward it
	//        to ensure undo debug checks work correctly
	
	undoCaptureBegin();
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
	undoCaptureFastForward();
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

static void backupSelectedNodes(const SceneEditor & sceneEditor, std::set<std::string> & capturedSelectedNodes)
{
	Assert(capturedSelectedNodes.empty());
	sceneEditor.scene.assignAutoGeneratedNodeNames();
	for (auto nodeId : sceneEditor.selection.selectedNodes)
	{
		auto & node = sceneEditor.scene.getNode(nodeId);
		capturedSelectedNodes.insert(node.name);
	}
}
static void restoreSelectedNodes(SceneEditor & sceneEditor, const std::set<std::string> & capturedSelectedNodes, const bool openNodes)
{
	for (auto & node_itr : sceneEditor.scene.nodes)
	{
		auto * node = node_itr.second;
		if (!node->name.empty() && capturedSelectedNodes.count(node_itr.second->name) != 0)
		{
			sceneEditor.selection.selectedNodes.insert(node->id);
			
			if (openNodes)
			{
				sceneEditor.nodeUi.nodesToOpen_deferred.insert(node->id);
			}
		}
	}
}

void SceneEditor::performAction_undo()
{
	if (undo.currentVersionIndex > 0)
	{
		auto & version = undo.versions[undo.currentVersionIndex - 1];
		auto & text = version.sceneText;
		
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
				tempScene.freeAllNodesAndComponents();
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
				#if !ENABLE_NODE_SELECTION_RESTORE_ON_UNDO_REDO
					std::set<std::string> capturedSelectedNodes;
					
					backupSelectedNodes(*this, capturedSelectedNodes);
				#endif
					{
						Assert(!deferred.isProcessing && deferred.isFlushed());
						deferred.isProcessing = true;
						{
							// note : we only need to insert the root node, as removeNodesToRemove recursively removes sub trees
							deferred.nodesToRemove.insert(scene.rootNodeId);
							
							removeNodesToRemove();
						}
						deferred.isProcessing = false;
						
						Assert(scene.nodes.empty());
						
						scene = tempScene;
						tempScene.nodes.clear();
					}
				#if !ENABLE_NODE_SELECTION_RESTORE_ON_UNDO_REDO
					restoreSelectedNodes(*this, capturedSelectedNodes, true);
				#endif
				
				#if ENABLE_NODE_SELECTION_RESTORE_ON_UNDO_REDO
					restoreSelectedNodes(*this, version.selectedNodes, true);
				#endif
					
					undo.currentVersion = text;
					undo.currentVersionIndex--;
				}
			}
		}
	}
}

void SceneEditor::performAction_redo()
{
	if (undo.currentVersionIndex + 1 < undo.versions.size())
	{
		auto & version = undo.versions[undo.currentVersionIndex + 1];
		auto & text = version.sceneText;
		
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
				tempScene.freeAllNodesAndComponents();
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
				#if !ENABLE_NODE_SELECTION_RESTORE_ON_UNDO_REDO
					std::set<std::string> capturedSelectedNodes;
					
					backupSelectedNodes(*this, capturedSelectedNodes);
				#endif
					{
						Assert(!deferred.isProcessing && deferred.isFlushed());
						deferred.isProcessing = true;
						{
							// note : we only need to insert the root node, as removeNodesToRemove recursively removes sub trees
							deferred.nodesToRemove.insert(scene.rootNodeId);
							
							removeNodesToRemove();
						}
						deferred.isProcessing = false;
						
						Assert(scene.nodes.empty());
						
						scene = tempScene;
						tempScene.nodes.clear();
					}
				#if !ENABLE_NODE_SELECTION_RESTORE_ON_UNDO_REDO
					restoreSelectedNodes(*this, capturedSelectedNodes, true);
				#endif
				
				#if ENABLE_NODE_SELECTION_RESTORE_ON_UNDO_REDO
					restoreSelectedNodes(*this, version.selectedNodes, true);
				#endif
					
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
					
					if (id == nullptr)
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
	
	auto * sceneNodeComponent = g_sceneNodeComponentMgr.createComponent(childNode->components.id);
	sceneNodeComponent->name = String::FormatC("Node %d", childNode->id);
	childNode->components.add(sceneNodeComponent);
	
	auto * transformComponent = g_transformComponentMgr.createComponent(childNode->components.id);
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
			deferred.nodesToAdd.push_back(childNode);
			deferred.nodesToSelect.insert(childNode->id);
		}
		deferredEnd(success);
	}
	
	return success;
}

void SceneEditor::performAction_remove()
{
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

void SceneEditor::performAction_remove(const int nodeId)
{
	deferredBegin();
	{
		if (nodeId != scene.rootNodeId)
		{
			deferred.nodesToRemove.insert(nodeId);
		}
	}
	deferredEnd(true);
}

bool SceneEditor::performAction_sceneAttach(const char * path, std::vector<int> * out_rootNodeIds)
{
	bool success = true;
	
	if (path != nullptr)
	{
		deferredBegin();
		{
			for (auto nodeId : selection.selectedNodes)
			{
				const int rootNodeId = attachScene(path, nodeId);
				
				success &= rootNodeId != -1;
				
				if (out_rootNodeIds != nullptr)
				{
					out_rootNodeIds->push_back(rootNodeId);
				}
			}
		}
		deferredEnd(success);
	}
	else
	{
	#if SCENEEDIT_USE_LIBNFD
		nfdchar_t * path = nullptr;

		if (NFD_OpenDialog(nullptr, nullptr, &path) == NFD_OKAY)
		{
			deferredBegin();
			{
				for (auto nodeId : selection.selectedNodes)
				{
					const int rootNodeId = attachScene(path, nodeId);
					
					success &= rootNodeId != -1;
					
					if (out_rootNodeIds != nullptr)
					{
						out_rootNodeIds->push_back(rootNodeId);
					}
				}
			}
			deferredEnd(success);
		}

		if (path != nullptr)
		{
			free(path);
			path = nullptr;
		}
	#else
		success &= false;
	#endif
	}
	
	if (success == false)
	{
		if (out_rootNodeIds != nullptr)
		{
			out_rootNodeIds->clear();
		}
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

bool SceneEditor::performAction_sceneImport(const char * path)
{
	bool success = true;
	
	if (path == nullptr)
	{
	#if SCENEEDIT_USE_LIBNFD
		nfdchar_t * path = nullptr;

		if (NFD_OpenDialog(nullptr, nullptr, &path) == NFD_OKAY)
		{
			deferredBegin();
			{
				for (auto nodeId : selection.selectedNodes)
					success &= addNodesFromScene(path, nodeId, false, nullptr) != -1;
			}
			deferredEnd(success);
		}

		if (path != nullptr)
		{
			free(path);
			path = nullptr;
		}
	#else
		success &= false;
	#endif
	}
	else
	{
		deferredBegin();
		{
			for (auto nodeId : selection.selectedNodes)
				success &= addNodesFromScene(path, nodeId, false, nullptr) != -1;
		}
		deferredEnd(success);
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
				if (nodeId == scene.rootNodeId)
					continue;
					
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

bool SceneEditor::performAction_parent(const int childNodeId, const int newParentId)
{
	deferredBegin();
	{
		deferred.nodesToParent.insert({ childNodeId, newParentId });
	}
	deferredEnd(true);
	
	return true;
}

void SceneEditor::drawSceneOpaque() const
{
	if (preview.drawScene)
	{
		g_modelComponentMgr.draw();
		
		g_gltfComponentMgr.drawOpaque();
	}
}

void SceneEditor::drawSceneOpaque_ForwardShaded() const
{
	if (preview.drawScene)
	{
		g_gltfComponentMgr.drawOpaque_ForwardShaded();
	}
}

void SceneEditor::drawSceneTranslucent() const
{
	if (preview.drawScene)
	{
		g_gltfComponentMgr.drawTranslucent();
	}
}

static void drawEditorGrid()
{
	gxPushMatrix();
	{
		gxScalef(10, 10, 10);
		
		setAlpha(255);
		
		setLumi(80);
		drawGrid3dLine(10, 10, 0, 2, true);
		
		setLumi(40);
		drawGrid3dLine(50, 50, 0, 2, true);
	}
	gxPopMatrix();
}

void SceneEditor::drawEditorGridOpaque() const
{
	if (visibility.drawGrid)
	{
		drawEditorGrid();
	}
}

void SceneEditor::drawEditorGridTranslucent() const
{
/*
	if (visibility.drawGrid)
	{
		pushLineSmooth(true);
		drawEditorGrid();
		popLineSmooth();
	}
*/
}

void SceneEditor::drawEditroGroundPlaneOpaque() const
{
	if (visibility.drawGroundPlane)
	{
		setLumi(200);
		fillCube(Vec3(), Vec3(100, .1f, 100));
	}
}

void SceneEditor::drawEditorNodeBoundingBox(const SceneNode & node) const
{
	const bool isHovered = node.id == hoverNodeId;
	const bool isSelected = selection.selectedNodes.count(node.id) != 0;
	
	Vec3 min(false);
	Vec3 max(false);
	if (getBoundingBoxForNode(node, min, max))
	{
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

void SceneEditor::drawEditorNodesOpaque() const
{
	// draw outline for hovered node

	if (hoverNodeId != -1)
	{
		gxPushMatrix();
		{
			auto & hoveredNode = scene.getNode(hoverNodeId);
			
			auto * sceneNodeComp = hoveredNode.components.find<SceneNodeComponent>();
			
			if (sceneNodeComp != nullptr)
			{
				gxMultMatrixf(sceneNodeComp->objectToWorld.m_v);
			}
			
			Vec3 min(false);
			Vec3 max(false);
			if (getBoundingBoxForNode(hoveredNode, min, max))
			{
				const Vec3 position = (min + max) / 2.f;
				const Vec3 size = (max - min) / 2.f;

			// todo : add special draw code to draw cube corners only using some nice colors. update all bounding box to use this (less intrusive) draw code
			
				setColor(colorWhite);
				lineCube(position, size);
			}
		}
		gxPopMatrix();
	}
}

void SceneEditor::drawEditorNodesTranslucent() const
{
	if (visibility.drawNodes)
	{
		pushBlend(BLEND_ADD);
		{
		/*
			// draw node markers
			
			beginCubeBatch();
			{
				for (auto & node_itr : scene.nodes)
				{
					auto * node = node_itr.second;
					
					Vec3 position;
					
					auto * sceneNodeComp = node->components.find<SceneNodeComponent>();
				
					if (sceneNodeComp != nullptr)
					{
						position = sceneNodeComp->objectToWorld.GetTranslation();
					}
					
					const bool isSelected = selection.selectedNodes.count(node->id) != 0;
					
					setColor(isSelected ? Color(100, 100, 0) : Color(255, 100, 100));
					fillCube(position, Vec3(.1f, .1f, .1f));
				}
			}
			endCubeBatch();
		*/
			
			// draw bounding boxes
			
			if (visibility.drawNodeBoundingBoxes)
			{
				for (auto & node_itr : scene.nodes)
				{
					gxPushMatrix();
					{
						auto * node = node_itr.second;
						
						auto * sceneNodeComp = node->components.find<SceneNodeComponent>();
					
						if (sceneNodeComp != nullptr)
						{
							gxMultMatrixf(sceneNodeComp->objectToWorld.m_v);
						}
						
						drawEditorNodeBoundingBox(*node);
					}
					gxPopMatrix();
				}
			}
		}
		popBlend();
	}
}

void SceneEditor::drawEditorGizmosOpaque(const bool depthObscuredPass) const
{
#if ENABLE_TRANSFORM_GIZMOS
	if (depthObscuredPass)
		pushDepthTest(true, DEPTH_GREATER, false);
	{
		transformGizmo.drawOpaque(depthObscuredPass);
	}
	if (depthObscuredPass)
		popDepthTest();
#endif
}

#include "components/cameraComponent.h"
#include "components/lightComponent.h"

void SceneEditor::drawEditorGizmosTranslucent() const
{
#if ENABLE_TRANSFORM_GIZMOS
	transformGizmo.drawTranslucent();
#endif

	// draw component specific gizmos
	
	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
	
	const Mat4x4 viewToWorld = worldToView.CalcInv();
	
	const Vec3 viewOrigin = viewToWorld.GetTranslation();
	
	Mat4x4 viewOrientation = viewToWorld;
	viewOrientation.SetTranslation(0, 0, 0);
	
	const bool doFade =
		framework.isStereoVr() ||
		camera.mode != Camera::kMode_Ortho;
		
	for (auto node_itr : scene.nodes)
	{
		auto * node = node_itr.second;
		
		gxPushMatrix();
		{
			auto * sceneNodeComp = node->components.find<SceneNodeComponent>();
			
			// draw at node location
			
			const Vec3 pos = sceneNodeComp->objectToWorld.GetTranslation();
			
			gxTranslatef(pos[0], pos[1], pos[2]);
			
			// orient to look at camera
			
			gxMultMatrixf(viewOrientation.m_v);
		
			GxTextureId simpleTextureId = 0;
			
			for (;;)
			{
				auto * cameraComp = node->components.find<CameraComponent>();
				
				if (cameraComp != nullptr && cameraComp->enabled)
				{
					simpleTextureId = getTexture("gizmo-camera.png");
					break;
				}
				
				auto * lightComp = node->components.find<LightComponent>();
				
				if (lightComp != nullptr && lightComp->enabled)
				{
					simpleTextureId = getTexture("gizmo-light.png");
					break;
				}
				
				// todo : add texture for audio emitter component
				
				// todo : add texture for vfx graph component
				
				break;
			}
			
			// draw a simple texture based gizmo when a texture is set
			
			if (simpleTextureId != 0)
			{
				pushCullMode(CULL_NONE, CULL_CCW);
				{
					gxSetTexture(simpleTextureId);
					gxBegin(GX_QUADS);
					{
						const float kFadeBeginDistance = 1.f;
						const float kFadeEndDistance = 4.f;
						const float kSize = .5f;
						
						const float distanceToView = (pos - viewOrigin).CalcSize();
						const float opacity =
							doFade
							? saturate<float>((distanceToView - kFadeBeginDistance) / (kFadeEndDistance - kFadeBeginDistance))
							: 1.f;
						
						gxColor4f(1, 1, 1, opacity);
						gxTexCoord2f(0.f, 0.f); gxVertex2f(-kSize, +kSize * 2.f);
						gxTexCoord2f(1.f, 0.f); gxVertex2f(+kSize, +kSize * 2.f);
						gxTexCoord2f(1.f, 1.f); gxVertex2f(+kSize, 0.f);
						gxTexCoord2f(0.f, 1.f); gxVertex2f(-kSize, 0.f);
					}
					gxEnd();
					gxSetTexture(0);
				}
				popCullMode();
			}
		}
		gxPopMatrix();
	}
}

void SceneEditor::drawEditorSelectedNodeLabels() const
{
	Mat4x4 projectionMatrix;
	Mat4x4 viewMatrix;
		
	if (framework.isStereoVr())
	{
		gxGetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
		gxGetMatrixf(GX_MODELVIEW, viewMatrix.m_v);
	}
	else
	{
		int viewportX;
		int viewportY;
		int viewportSx;
		int viewportSy;
		getEditorViewport(viewportX, viewportY, viewportSx, viewportSy);
		
		camera.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);
		projectionMatrix = Mat4x4(true)
			.Translate(viewportX, viewportY, 0)
			.Mul(projectionMatrix);

		camera.calculateViewMatrix(viewMatrix);
	}
	
	const Mat4x4 modelViewProjection = projectionMatrix * viewMatrix;
	
	setFont("calibri.ttf");
	
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

void SceneEditor::drawView2d() const
{
	drawEditorSelectedNodeLabels();
}

void SceneEditor::drawView3dOpaque() const
{
	drawEditorGridOpaque();
	drawEditroGroundPlaneOpaque();
	drawEditorNodesOpaque();
}

void SceneEditor::drawView3dOpaque_ForwardShaded() const
{
	Mat4x4 worldToView;
	gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
	const Mat4x4 viewToWorld = worldToView.CalcInv();
	
	drawEditorGizmosOpaque(true);
	drawEditorGizmosOpaque(false);
				
// todo : draw node name labels here? we need to prevent them from obscuring the interactive ring. also would be nice if they appear in vr mode

#if false // todo : make this into a nice card that we can inspect in VR when we want to
	// draw node details
	
	for (auto nodeId : selection.selectedNodes)
	{
		gxPushMatrix();
		{
			auto & node = scene.getNode(nodeId);
			
			auto * sceneNodeComp = node.components.find<SceneNodeComponent>();
			auto translation = sceneNodeComp->objectToWorld.GetTranslation();
			
			gxTranslatef(translation[0], translation[1], translation[2]);
			
			// rotate to face the camera
			const Vec3 forwardVector = viewToWorld.GetAxis(2);
			const float rotationY = atan2f(forwardVector[0], forwardVector[2]);
			gxRotatef(Calc::RadToDeg(rotationY), 0, 1, 0);
			
			// flip Y axis to make sure text renders correctly
			gxScalef(1, -1, 1);
			
			pushFontMode(FONT_SDF); // todo : font spec once
			setFont("calibri.ttf");
			
			const float kFontSize = .2f;
			
			float y = 0.f;
			
			pushBlend(BLEND_ALPHA); // todo : opaque font rendering support. enforce alpha = 100% or 0% but no in-between values. but what about depth write? discard pixels?
			beginTextBatch();
			{
				for (auto * component = node.components.head; component != nullptr; component = component->next_in_set)
				{
					auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
					
					setColor(100, 100, 200);
					drawText(0, y, kFontSize, 0, 0, "%s", componentType->typeName);
					y += kFontSize;
					
					for (auto * member = componentType->members_head; member != nullptr; member = member->next)
					{
						setColor(200, 100, 100);
						drawText(kFontSize, y, kFontSize, 0, 0, "%s", member->name);
						y += kFontSize;
					}
				}
			}
			endTextBatch();
			popBlend();
			
			popFontMode();
		}
		gxPopMatrix();
	}
#endif

	interactiveRing.drawOpaque(*this);
}

void SceneEditor::drawView3dTranslucent() const
{
	drawEditorGridTranslucent();
	
	drawEditorGizmosTranslucent();
	
	drawEditorNodesTranslucent();
}

bool SceneEditor::loadSceneFromLines_nonDestructive(std::vector<std::string> & lines, const char * basePath)
{
	Scene tempScene;
	tempScene.nodeIdAllocator = &scene;
	tempScene.createRootNode();

	LineReader line_reader(lines, 0, 0);
	
	if (parseSceneFromLines(*typeDB, line_reader, basePath, tempScene) == false)
	{
		logError("failed to load scene from lines");
		tempScene.freeAllNodesAndComponents();
		return false;
	}
	else if (!tempScene.validate())
	{
		logError("failed to validate scene");
		tempScene.freeAllNodesAndComponents();
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
			return false;
		}
		else
		{
			deferredBegin();
			{
				// empty the scene, except for the root node
				
				for (auto & node_itr : scene.nodes)
				{
					auto * node = node_itr.second;
					
					if (node->id != scene.rootNodeId)
					{
						deferred.nodesToRemove.insert(node->id);
					}
				}
				
				// insert nodes from the loaded scene, except for the root node
				
				for (auto & node_itr : tempScene.nodes)
				{
					auto * node = node_itr.second;
					
					if (node->id == tempScene.rootNodeId)
					{
						// delete the node if it's the root node
						
						node->freeComponents();
						
						delete node;
						node = nullptr;
					}
					else
					{
						// if not the root node, keep it and add it to the scene
						
						deferred.nodesToAdd.push_back(node);
						
						// if parented to the loaded scene's root node, reparent it
						
						if (node->parentId == tempScene.rootNodeId)
							node->parentId = scene.rootNodeId;
					}
				}
				
				tempScene.nodes.clear();
			}
			deferredEnd(true);
			
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
	
	undoReset();
}

void SceneEditor::updateFilePaths(Scene & scene, const TypeDB * typeDB, const char * oldBasePath, const char * newBasePath)
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
			auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
			
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

std::string SceneEditor::makePathRelativeToDocumentPath(const char * path) const
{
	if (documentInfo.path.empty())
	{
		return path;
	}
	else
	{
		auto isAbsolutePath = [](const char * path)
		{
			return
				path[0] == '/' ||
				strchr(path, ':') != nullptr;
		};
		
		Assert(isAbsolutePath(documentInfo.path.c_str()));
		
		const std::string absolutePath =
			isAbsolutePath(path)
				? path
				: Path::MakeAbsolute(getDirectory(), path);
				
		return Path::MakeRelative(Path::GetDirectory(documentInfo.path), absolutePath);
	}
}

void SceneEditor::initTemplateUi()
{
	// generate a structured tree of templates, based on template paths
	
	templateUi = TemplateUi();
	
	// 1. determine the editor data path and derive the templates path from this
	
	const auto editorDataPath = Path::GetDirectory(framework.resolveResourcePath("gizmo-light.png"));
	
	const auto templatesPath = editorDataPath + "/templates";
	
	// 2. scan files inside the templates path and make the paths relative to the editor data path
	
	auto files = listFiles(templatesPath.c_str(), false);
	
	for (auto & file : files)
	{
		file = Path::MakeRelative(editorDataPath, file);
	}
	
	// sort the files by name so the structured tree we'll generated will be sorted as well
	
	std::sort(files.begin(), files.end());
	
	// 3. generate the tree
	
	for (auto & file : files)
	{
		Assert(Path::GetExtension(file, true) == "txt");
		
		if (Path::GetExtension(file, true) != "txt")
			continue;
		
		// strip the directory from the path to get the filename
		
		const char * begin = file.c_str();
		
		begin = strchr(begin, '/');
		
		Assert(begin != nullptr);
		if (begin == nullptr)
			continue;
			
		begin++;
		
		TemplateUi::TemplateElem * currentElem = &templateUi.rootTemplateElem;
		
		for (;;)
		{
			// template filenames look like 'light-point.txt'
			// 'light' would be the name of the sub-tree. 'point' the name of the template elem
			
			// is this a sub-tree or is a leaf node?
			
			const char * end = strchr(begin, '-');
			
			if (end != nullptr)
			{
				// sub-tree case: find or create sub-tree within the current template elem and traverse
				
				const size_t name_length = end - begin;
				char * name = (char*)alloca((end - begin + 1) * sizeof(char));
				memcpy(name, begin, name_length * sizeof(char));
				name[name_length] = 0;
				
				begin = end + 1;
				
				TemplateUi::TemplateElem * childElem = nullptr;
				
				for (auto & child : currentElem->children)
				{
					if (child.name == name)
						childElem = &child;
				}
				
				if (childElem == nullptr)
				{
					currentElem->children.resize(currentElem->children.size() + 1);
					currentElem = &currentElem->children.back();
					currentElem->name = name;
				}
				else
					currentElem = childElem;
			}
			else
			{
				// leaf case
				
				auto name = Path::StripExtension(begin);
				
				currentElem->children.resize(currentElem->children.size() + 1);
				currentElem = &currentElem->children.back();
				
				currentElem->name = name;
				currentElem->path = file;
				
				break;
			}
		}
	}
}
			
//

#if SCENEEDIT_USE_IMGUIFILEDIALOG

static std::string s_fileDialogResult;

static void openImGuiOpenDialog(const char * key, const char * caption, const char * filter, const char * initialPath)
{
	ImGuiFileDialog::Instance()->OpenModal(
		key,
		caption,
		filter,
		initialPath);
}

static void openImGuiSaveDialog(const char * key, const char * caption, const char * filter, const char * initialPath)
{
	ImGuiFileDialog::Instance()->OpenModal(
		key,
		caption,
		filter,
		initialPath,
		1,
		nullptr,
		ImGuiFileDialogFlags_ConfirmOverwrite);
}

static const char * doImGuiFileDialog(const SceneEditor & sceneEditor, const char * key)
{
	const char * result = nullptr;
	
	const ImVec2 minSize(
		ImGui::GetIO().DisplaySize.x / 2,
		ImGui::GetIO().DisplaySize.y / 2);
	const ImVec2 maxSize(
		ImGui::GetIO().DisplaySize.x,
		ImGui::GetIO().DisplaySize.y);
	
	if (ImGuiFileDialog::Instance()->Display(key, ImGuiWindowFlags_NoCollapse, minSize, maxSize))
	{
		if (ImGuiFileDialog::Instance()->IsOk())
		{
		// fixme : selection method doesn't work well.. when manually entering filename,
		//         and hitting enter, it will not be set to the filename the user entered..
		//         instead, it's the initial filename (if any). this causes files to be
		//         unexpectedly overwritten
		
		#if defined(DEBUG)
			auto filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
			auto filePath = ImGuiFileDialog::Instance()->GetCurrentPath();
			auto fileName = ImGuiFileDialog::Instance()->GetCurrentFileName();
			auto path = ImGuiFileDialog::Instance()->GetCurrentPath();
			
			logDebug("filePathName: %s", filePathName.c_str());
			logDebug("filePath: %s", filePath.c_str());
			logDebug("fileName: %s", fileName.c_str());
			logDebug("path: %s", path.c_str());
		#endif
		
		#if 1
			s_fileDialogResult = ImGuiFileDialog::Instance()->GetFilePathName();
			
			result = s_fileDialogResult.c_str();
		#else
			auto selection = ImGuiFileDialog::Instance()->GetSelection();
			
			if (!selection.empty())
			{
				s_fileDialogResult = selection.begin()->second.c_str();
				
				result = s_fileDialogResult.c_str();
			}
		#endif
		}
		
		ImGuiFileDialog::Instance()->Close();
	}
	
	return result;
}

#endif
