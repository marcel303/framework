#include "cameraComponent.h"
#include "modelComponent.h"
#include "parameterComponent.h"
#include "parameterComponentUi.h"
#include "parameterUi.h"
#include "transformComponent.h"

#include "component.h"
#include "componentPropertyUi.h"
#include "componentType.h"
#include "framework.h"
#include "framework-camera.h"
#include "imgui-framework.h"
#include "Quat.h"
#include "raycast.h"
#include "scene.h"
#include "scene_fromText.h"
#include "sceneNodeComponent.h"
#include "StringEx.h"
#include "TextIO.h"
#include "transformGizmos.h"

#include "helpers.h"
#include "lineReader.h"
#include "lineWriter.h"
#include "template.h"

#include <algorithm>
#include <limits>
#include <set>
#include <typeindex>

#include "componentJson.h" // todo : remove
#include <json.hpp> // todo : remove

#define ENABLE_TRANSFORM_GIZMOS 1

#define ENABLE_QUAT_FIXUP 1

/*

todo :

+ add libreflection-jsonio
- remove json references from scene edit
+ add general monitor camera. or extend framework's Camera3d to support perspective, orbit and ortho modes
- avoid UI from jumping around
	- add independent scrolling area for scene structure
	- add independent scrolling area for selected node
+ use scene structure tree to select nodes. remove inline editing
- add lookat/focus option to scene structure view
	- as a context menu
+ separate node traversal/scene structure view from node editor

*/

extern void test_templates();
extern bool test_scenefiles();
extern bool test_templateEditor();
extern void test_reflection_1();
extern void test_bindObjectToFile();

static const int VIEW_SX = 1200;
static const int VIEW_SY = 800;

// todo : move these instances to helpers.cpp
extern TransformComponentMgr s_transformComponentMgr;
extern ModelComponentMgr s_modelComponentMgr;
extern ParameterComponentMgr s_parameterComponentMgr;

// todo : move to helpers
static bool node_to_clipboard_text(const SceneNode & node, std::string & text);
static bool node_from_clipboard_text(const char * text, SceneNode & node);

//

struct Renderer
{
	enum Mode
	{
		kMode_Wireframe,
		kMode_Colors,
		kMode_Normals,
		kMode_Lit,
		kMode_LitWithShadows
	};
	
	ParameterMgr parameterMgr;
	
	ParameterEnum * mode = nullptr;
	
	bool init()
	{
		bool result = true;

		parameterMgr.init("renderer");
		
		mode = parameterMgr.addEnum("mode", kMode_Colors,
			{
				{ "Wireframe", kMode_Wireframe },
				{ "Colors", kMode_Colors },
				{ "Normals", kMode_Normals },
				{ "Lit", kMode_Lit },
				{ "Lit + Shadows", kMode_LitWithShadows }
			});
		
		return result;
	}
};

//

struct SceneEditor
{
	Scene scene; // todo : this should live outside the editor, but referenced
	
	Camera camera; // todo : this should live outside the editor, but referenced
	bool cameraIsActive = false;
	
	std::set<int> selectedNodes;
	
	int nodeToGiveFocus = -1;
	bool enablePadGizmo = false;
	
#if ENABLE_TRANSFORM_GIZMOS
	TranslationGizmo translationGizmo;
#endif

	Renderer renderer; // todo : this should live outside the editor
	
	struct
	{
		bool drawGroundPlane = true;
		bool drawNodes = true;
		bool drawNodeBoundingBoxes = true;
	} visibility;
	
	struct
	{
		bool tickScene = false;
		float tickMultiplier = 1.f;
		
		bool drawScene = false;
	} preview;
	
	FrameworkImGuiContext guiContext;
	
	struct
	{
		std::set<int> nodesToRemove;
		std::vector<SceneNode*> nodesToAdd;
		
		bool isFlushed() const
		{
			return
				nodesToRemove.empty() &&
				nodesToAdd.empty();
		}
	} deferred;
	
	static const int kMaxNodeDisplayNameFilter = 100;
	static const int kMaxComponentTypeNameFilter = 100;
	
	struct
	{
		char nodeDisplayNameFilter[kMaxNodeDisplayNameFilter] = { };
		char componentTypeNameFilter[kMaxComponentTypeNameFilter] = { };
		std::set<int> visibleNodes;
		std::set<int> openNodes;
	} nodeUi;
	
	static const int kMaxParameterFilter = 100;
	
	struct
	{
		char component_filter[kMaxParameterFilter] = { };
		char parameter_filter[kMaxParameterFilter] = { };
		
		bool showAnonymousComponents = false;
	} parameterUi;
	
	//
	
	SceneEditor()
	{
		camera.mode = Camera::kMode_Orbit;
		camera.orbit.distance = -4.f;
		camera.ortho.scale = 4.f;
		camera.firstPerson.position = Vec3(0, 1, -2);
	}
	
	void init()
	{
		guiContext.init();
		
		scene.createRootNode();
		
		renderer.init();
	}
	
	void shut()
	{
		guiContext.shut();
	}
	
	SceneNode * raycast(Vec3Arg rayOrigin, Vec3Arg rayDirection)
	{
		SceneNode * result = nullptr;
		float bestDistance = std::numeric_limits<float>::max();
		
		for (auto & nodeItr : scene.nodes)
		{
			auto & node = *nodeItr.second;
			
			auto * sceneNodeComp = node.components.find<SceneNodeComponent>();
			auto * modelComponent = node.components.find<ModelComponent>();
			
			Assert(sceneNodeComp != nullptr);
			
			if (sceneNodeComp != nullptr && modelComponent != nullptr)
			{
				auto & objectToWorld = sceneNodeComp->objectToWorld;
				auto worldToObject = objectToWorld.CalcInv();
				
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
						bestDistance = distance;
						
						result = &node;
					}
				}
			}
		}
		
		return result;
	}
	
	void deferredBegin()
	{
		Assert(deferred.isFlushed());
	}
	
	void deferredEnd()
	{
		addNodesToAdd();
		
		removeNodesToRemove();
	}
	
	void removeNodeTraverse(const int nodeId, const bool removeFromParent)
	{
		auto nodeItr = scene.nodes.find(nodeId);
		
		Assert(nodeItr != scene.nodes.end());
		if (nodeItr != scene.nodes.end())
		{
			auto & node = *nodeItr->second;
			
			if (removeFromParent == false)
			{
				for (auto childNodeId : node.childNodeIds)
				{
					removeNodeTraverse(childNodeId, removeFromParent);
				}
				
				node.childNodeIds.clear();
			}
			else
			{
				while (!node.childNodeIds.empty())
				{
					removeNodeTraverse(node.childNodeIds.front(), removeFromParent);
				}
				
				Assert(node.childNodeIds.empty());
			}
			
			if (removeFromParent && node.parentId != -1)
			{
				auto parentNodeItr = scene.nodes.find(node.parentId);
				Assert(parentNodeItr != scene.nodes.end());
				if (parentNodeItr != scene.nodes.end())
				{
					auto & parentNode = *parentNodeItr->second;
					auto childNodeItr = std::find(parentNode.childNodeIds.begin(), parentNode.childNodeIds.end(), node.id);
					Assert(childNodeItr != parentNode.childNodeIds.end());
					if (childNodeItr != parentNode.childNodeIds.end())
						parentNode.childNodeIds.erase(childNodeItr);
				}
			}
			
			/*
			// optimize : set parent id to -1 to avoid the child from removing itself from the parent's child list. since we are removing the parent itself here the child doesn't need to do this itself
			// todo : pass a boolean to the child instructing it to remove itself or not ?
			for (auto childNodeId : node.childNodeIds)
			{
				auto childNodeItr = scene.nodes.find(childNodeId);
		
				Assert(childNodeItr != scene.nodes.end());
				if (childNodeItr != scene.nodes.end())
				{
					auto & childNode = childNodeItr->second;
					childNode.parentId = -1;
				}
				
				removeNodeTraverse(childNodeId);
			}
			*/
			
			node.freeComponents();
			
			delete &node;
			
			scene.nodes.erase(nodeItr);
			
			selectedNodes.erase(nodeId);
			
			deferred.nodesToRemove.erase(nodeId);
		}
	}
	
	void addNodesToAdd()
	{
		if (deferred.nodesToAdd.empty() == false)
		{
			selectedNodes.clear();
			
			for (SceneNode * node : deferred.nodesToAdd)
			{
				scene.nodes[node->id] = node;
				
				auto parentNode_itr = scene.nodes.find(node->parentId);
				Assert(parentNode_itr != scene.nodes.end());
				
				// insert the node into the scene
				auto * parentNode = parentNode_itr->second;
				parentNode->childNodeIds.push_back(node->id);
				
				// select the newly added child node
				selectedNodes.insert(node->id);
				nodeToGiveFocus = node->id;
				
				
				// when a display name filter is set the node would just disappear. set the filter to the name of the newly
				// added node in this case, to avoid confusion
				if (nodeUi.nodeDisplayNameFilter[0] != 0)
				{
					strcpy_s(nodeUi.nodeDisplayNameFilter, sizeof(nodeUi.nodeDisplayNameFilter), node->displayName.c_str());
				}
			}
			
			deferred.nodesToAdd.clear();
		}
	}
	
	void removeNodesToRemove()
	{
		while (!deferred.nodesToRemove.empty())
		{
			auto nodeToRemoveItr = deferred.nodesToRemove.begin();
			auto nodeId = *nodeToRemoveItr;
			
			removeNodeTraverse(nodeId, true);
		}

		Assert(deferred.nodesToRemove.empty());
	#if defined(DEBUG)
		for (auto & selectedNodeId : selectedNodes)
			Assert(scene.nodes.find(selectedNodeId) != scene.nodes.end());
	#endif
	}
	
	void editNode(const int nodeId)
	{
		const bool do_filter = nodeUi.nodeDisplayNameFilter[0] != 0;
		
		auto nodeItr = scene.nodes.find(nodeId);
		
		Assert(nodeItr != scene.nodes.end());
		if (nodeItr == scene.nodes.end())
			return;
		
		auto & node = *nodeItr->second;
		
		const bool passes_filter = do_filter == false || strcasestr(node.displayName.c_str(), nodeUi.nodeDisplayNameFilter) != nullptr;
		
		// todo : make a separate function to edit a data structure (recursively)
		
		if (passes_filter)
		{
			ImGui::PushID(nodeId);
			
			ImGui::Text("Node");
			ImGui::Indent();
			{
				char name[256];
				sprintf_s(name, sizeof(name), "%s", node.displayName.c_str());
				if (ImGui::InputText("name", name, sizeof(name)))
					node.displayName = name;
			}
			ImGui::Unindent();

			if (node.components.head == nullptr)
			{
				// no components exist, but we still need some area for the user to click to add nodes and components
				
				ImGui::Text("(no components");
				
				if (ImGui::BeginPopupContextItem("NodeMenu"))
				{
					doNodeContextMenu(node, nullptr);

					ImGui::EndPopup();
				}
			}
			else
			{
				for (auto * component = node.components.head; component != nullptr; )
				{
					ImGui::PushID(component);
					{
						// note : we group component properties here, so the node context menu can be opened by
						// right clicking anywhere inside this group
						
						ImGui::BeginGroup();
						{
							auto * componentType = findComponentType(component->typeIndex());
							
							Assert(componentType != nullptr);
							if (componentType != nullptr)
							{
								ImGui::LabelText("", "%s", componentType->typeName);
								ImGui::Indent();
								
								bool isSet = true;
								void * changedMemberObject = nullptr;
								
								if (doReflection_StructuredType(g_typeDB, *componentType, component, isSet, nullptr, &changedMemberObject))
								{
									// signal component one of its properties has changed
									component->propertyChanged(changedMemberObject);
								}
								
								ImGui::Unindent();
							}
						}
						ImGui::EndGroup();
					
						// see if we should open the node context menu
						
						NodeContextMenuResult result = kNodeContextMenuResult_None;
					
						if (ImGui::BeginPopupContextItem("NodeMenu"))
						{
							result = doNodeContextMenu(node, component);

							ImGui::EndPopup();
						}
					
						if (result == kNodeContextMenuResult_ComponentShouldBeRemoved)
						{
							auto * next = component->next_in_set;
							
							freeComponentInComponentSet(node.components, component);
							
							component = next;
						}
						else
						{
							component = component->next_in_set;
						}
					}
					ImGui::PopID();
				}
			}
			
			ImGui::PopID();
		}
	}
	
	void pasteNodeFromClipboard(const int parentId)
	{
		SceneNode * childNode = new SceneNode();
		childNode->id = scene.allocNodeId();
		childNode->parentId = parentId;
		childNode->displayName = String::FormatC("Node %d", childNode->id);
		
		const char * text = SDL_GetClipboardText();
		
		if (node_from_clipboard_text(text, *childNode) == false)
		{
			delete childNode;
			childNode = nullptr;
		}
		else
		{
			if (childNode->components.find<SceneNodeComponent>() == nullptr)
				childNode->components.add(new SceneNodeComponent());
			
			if (childNode->initComponents() == false)
			{
				childNode->freeComponents();
				
				delete childNode;
				childNode = nullptr;
			}
			else
			{
				deferred.nodesToAdd.push_back(childNode);
			}
		}
		
		SDL_free((void*)text);
		text = nullptr;
	}

	enum NodeStructureContextMenuResult
	{
		kNodeStructureContextMenuResult_None,
		kNodeStructureContextMenuResult_NodeCopy,
		kNodeStructureContextMenuResult_NodePaste,
		kNodeStructureContextMenuResult_NodeQueuedForRemove,
		kNodeStructureContextMenuResult_NodeAdded
	};
	
	NodeStructureContextMenuResult doNodeStructureContextMenu(SceneNode & node)
	{
		//logDebug("context window for %d", node.id);
		
		NodeStructureContextMenuResult result = kNodeStructureContextMenuResult_None;

		if (ImGui::MenuItem("Copy"))
		{
			result = kNodeStructureContextMenuResult_NodeCopy;
			
			std::string text;
			if (node_to_clipboard_text(node, text))
			{
				SDL_SetClipboardText(text.c_str());
			}
		}
		
		if (ImGui::MenuItem("Paste as child", nullptr, false, SDL_HasClipboardText()))
		{
			result = kNodeStructureContextMenuResult_NodePaste;
			
			pasteNodeFromClipboard(node.id);
		}
		
		if (ImGui::MenuItem("Paste as sibling", nullptr, false, SDL_HasClipboardText()))
		{
			result = kNodeStructureContextMenuResult_NodePaste;
			
			pasteNodeFromClipboard(node.parentId);
		}
		
		if (ImGui::MenuItem("Remove"))
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
			childNode->displayName = String::FormatC("Node %d", childNode->id);
			
			childNode->components.add(new SceneNodeComponent());
			
			if (childNode->initComponents() == false)
			{
				childNode->freeComponents();
				
				delete childNode;
				childNode = nullptr;
			}
			else
			{
				deferred.nodesToAdd.push_back(childNode);
			}
		}
		
		return result;
	}
	
	enum NodeContextMenuResult
	{
		kNodeContextMenuResult_None,
		kNodeContextMenuResult_ComponentShouldBeRemoved,
		kNodeContextMenuResult_ComponentAdded
	};
	
	NodeContextMenuResult doNodeContextMenu(SceneNode & node, ComponentBase * component)
	{
		//logDebug("context window for %d", node.id);
		
		NodeContextMenuResult result = kNodeContextMenuResult_None;
		
		if (component != nullptr)
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
				bool isAdded = false;
				
				for (auto * component = node.components.head; component != nullptr; component = component->next_in_set)
					if (component->typeIndex() == componentType->componentMgr->typeIndex())
						isAdded = true;
				
				if (isAdded == false)
				{
					char text[256];
					sprintf_s(text, sizeof(text), "Add %s", componentType->typeName);
					
					if (ImGui::MenuItem(text))
					{
						result = kNodeContextMenuResult_ComponentAdded;
						
						auto * component = componentType->componentMgr->createComponent(nullptr);
						
						if (component->init())
							node.components.add(component);
						else
							componentType->componentMgr->destroyComponent(component);
					}
				}
			}
			
			ImGui::EndMenu();
		}
		
		return result;
	}
	
	void editNodeStructure_traverse(const int nodeId)
	{
		const bool do_filter = nodeUi.nodeDisplayNameFilter[0] != 0;
		
		auto nodeItr = scene.nodes.find(nodeId);
		
		Assert(nodeItr != scene.nodes.end());
		if (nodeItr == scene.nodes.end())
			return;
		
		auto & node = *nodeItr->second;
		
		//
		
		const bool isSelected = selectedNodes.count(node.id) != 0;
		const bool isLeaf = node.childNodeIds.empty();
		const bool isRoot = node.parentId == -1;
		
		ImGui::PushID(&node);
		{
			if (nodeToGiveFocus == nodeId)
			{
				nodeToGiveFocus = -1;
				ImGui::SetScrollHereY();
			}
			
			if (nodeUi.openNodes.count(node.id) != 0)
				ImGui::SetNextTreeNodeOpen(true);

			const bool isOpen = ImGui::TreeNodeEx(&node,
				(ImGuiTreeNodeFlags_OpenOnArrow * 1) |
				(ImGuiTreeNodeFlags_Selected * isSelected) |
				(ImGuiTreeNodeFlags_Leaf * isLeaf) |
				(ImGuiTreeNodeFlags_DefaultOpen * isRoot) |
				(ImGuiTreeNodeFlags_FramePadding * 0), "%s", node.displayName.c_str());
			const bool isClicked = ImGui::IsItemClicked();

			if (isClicked)
			{
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
				for (auto & childNodeId : node.childNodeIds)
				{
					auto childNodeItr = scene.nodes.find(childNodeId);
					
					Assert(childNodeItr != scene.nodes.end());
					if (childNodeItr != scene.nodes.end())
					{
						auto * childNode = childNodeItr->second;
						
						if (do_filter && nodeUi.visibleNodes.count(childNode->id) == 0)
							continue;

						editNodeStructure_traverse(childNodeId);
					}
				}
			
				ImGui::TreePop();
			}
		}
		ImGui::PopID();
	}
	
	void addNodeFromTemplate_v1(Vec3Arg position, const AngleAxis & angleAxis, const int parentId)
	{
		SceneNode * node = new SceneNode();
		node->id = scene.allocNodeId();
		node->parentId = parentId;
		node->displayName = String::FormatC("Node %d", node->id);
		
		node->components.add(new SceneNodeComponent());
		
		auto * modelComp = s_modelComponentMgr.createComponent(nullptr);
		modelComp->filename = "model.txt";
		modelComp->scale = .01f;
		node->components.add(modelComp);
		
		auto * transformComp = s_transformComponentMgr.createComponent(nullptr);
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
			
			auto parentNode_itr = scene.nodes.find(parentId);
			Assert(parentNode_itr != scene.nodes.end());
			if (parentNode_itr != scene.nodes.end())
			{
				auto & parentNode = *parentNode_itr->second;
				
				parentNode.childNodeIds.push_back(node->id);
			}
		}
	}
	
	int addNodeFromTemplate_v2(Vec3Arg position, const AngleAxis & angleAxis, const int parentId)
	{
		Template t;
		
		if (!loadTemplateWithOverlaysFromFile("textfiles/base-entity-v1-overlay.txt", t, false))
			return -1;
		
		//
		
		SceneNode * node = new SceneNode();
		node->id = scene.allocNodeId();
		node->parentId = parentId;
		node->displayName = String::FormatC("Node %d", node->id);
		
		node->components.add(new SceneNodeComponent());
		
		bool init_ok = true;
		
		init_ok &= instantiateComponentsFromTemplate(g_typeDB, t, node->components);
		
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
		
		scene.nodes[node->id] = node;
		
		//
		
		auto parentNode_itr = scene.nodes.find(parentId);
		Assert(parentNode_itr != scene.nodes.end());
		if (parentNode_itr != scene.nodes.end())
		{
			auto & parentNode = *parentNode_itr->second;
			
			parentNode.childNodeIds.push_back(node->id);
		}
		
		return node->id;
	}
	
	void tickEditor(const float dt, bool & inputIsCaptured)
	{
		//if (mouse.isCaptured == false)
		{
			guiContext.processBegin(dt, VIEW_SX, VIEW_SY, inputIsCaptured);
			{
				ImGui::SetNextWindowPos(ImVec2(4, 20 + 4 + 4));
				ImGui::SetNextWindowSize(ImVec2(370, VIEW_SY - 20 - 4 - 4));
				if (ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					if (ImGui::CollapsingHeader("Visibility", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::Indent();
						{
							ImGui::Checkbox("Draw ground plane", &visibility.drawGroundPlane);
							ImGui::Checkbox("Draw nodes", &visibility.drawNodes);
							ImGui::Checkbox("Draw node bounding boxes", &visibility.drawNodeBoundingBoxes);
						}
						ImGui::Unindent();
					}
					
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
					
					if (ImGui::CollapsingHeader("Scene structure", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::InputText("Display name", nodeUi.nodeDisplayNameFilter, kMaxNodeDisplayNameFilter);
						ImGui::InputText("Component type", nodeUi.componentTypeNameFilter, kMaxComponentTypeNameFilter);
						
						ImGui::BeginChild("Scene structure", ImVec2(0, 140), ImGuiWindowFlags_AlwaysVerticalScrollbar);
						{
							// determine the set of visible nodes, given any filters applied to the list
							// note that when a child node is visible, all its elders are visible as well
							
							if (nodeUi.nodeDisplayNameFilter[0] != 0)
							{
								for (auto & node_itr : scene.nodes)
								{
									auto nodeId = node_itr.first;
									auto * node = node_itr.second;
									
									if (nodeUi.visibleNodes.count(nodeId) != 0)
										continue;
									
									const bool is_visible = strcasestr(node->displayName.c_str(), nodeUi.nodeDisplayNameFilter);
									
									if (is_visible)
									{
										nodeUi.visibleNodes.insert(nodeId);
										
										int parentId = node->parentId;
										
										while (parentId != -1)
										{
											if (nodeUi.visibleNodes.count(parentId) != 0)
												break;
											
											nodeUi.visibleNodes.insert(parentId);
											
											auto parentNode_itr = scene.nodes.find(parentId);
											Assert(parentNode_itr != scene.nodes.end());
											if (parentNode_itr != scene.nodes.end())
												parentId = parentNode_itr->second->parentId;
										}
									}
								}
							}
							
							// determine the set of 'open' nodes. where open means the tree node entry for the
							// node is unfolded. open nodes are all selected nodes and any of its elders
							
							for (auto & nodeId : selectedNodes)
							{
								auto node_itr = scene.nodes.find(nodeId);
								Assert(node_itr != scene.nodes.end());
								if (node_itr == scene.nodes.end())
									continue;
								
								auto * node = node_itr->second;
								int parentId = node->parentId;
						
								while (parentId != -1)
								{
									if (nodeUi.openNodes.count(parentId) != 0)
										break;
									
									nodeUi.openNodes.insert(parentId);
									
									auto parentNode_itr = scene.nodes.find(parentId);
									Assert(parentNode_itr != scene.nodes.end());
									if (parentNode_itr != scene.nodes.end())
										parentId = parentNode_itr->second->parentId;
								}
							}
							
							deferredBegin();
							{
								editNodeStructure_traverse(scene.rootNodeId);
							}
							deferredEnd();
							
							nodeUi.visibleNodes.clear();
							nodeUi.openNodes.clear();
						}
						ImGui::EndChild();
					}
					
					if (ImGui::CollapsingHeader("Selected node(s)", ImGuiTreeNodeFlags_DefaultOpen))
					{
						deferredBegin();
						ImGui::BeginChild("Selected nodes", ImVec2(0, 300), ImGuiWindowFlags_AlwaysVerticalScrollbar);
						{
							for (auto & selectedNodeId : selectedNodes)
							{
								editNode(selectedNodeId);
							}
						}
						ImGui::EndChild();
						deferredEnd();
						
						// one or more transforms may have been edited or invalidated due to a parent node being invalidated
						// ensure the transforms are up to date by recalculating them. this is needed for transform gizmos
						// to work
						s_transformComponentMgr.calculateTransforms(scene);
					}
				}
				ImGui::End();
				
				//
				
			#if 1
				if (ImGui::BeginMainMenuBar())
				{
					if (ImGui::BeginMenu("Renderer"))
					{
						doParameterUi(renderer.parameterMgr, nullptr, false);
						
						ImGui::EndMenu();
					}
					
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
			#endif
			
				guiContext.updateMouseCursor();
			}
			guiContext.processEnd();
		}
		
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_a) && keyboard.isDown(SDLK_LGUI))
		{
			inputIsCaptured = true;
			preview.tickMultiplier *= 1.25f;
		}
		
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_z) && keyboard.isDown(SDLK_LGUI))
		{
			inputIsCaptured = true;
			preview.tickMultiplier /= 1.25f;
		}
		
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_c) && keyboard.isDown(SDLK_LGUI))
		{
			inputIsCaptured = true;
			
			if (selectedNodes.empty() == false)
			{
				auto nodeId = *selectedNodes.begin(); // todo : handle multiple nodes
				auto node_itr = scene.nodes.find(nodeId);
				Assert(node_itr != scene.nodes.end());
				auto * node = node_itr->second;
				std::string text;
				if (node_to_clipboard_text(*node, text))
					SDL_SetClipboardText(text.c_str());
			}
		}
		
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_v) && keyboard.isDown(SDLK_LGUI))
		{
			inputIsCaptured = true;
			deferredBegin();
			pasteNodeFromClipboard(scene.rootNodeId);
			deferredEnd();
		}
		
		// transform mouse coordinates into a world space direction vector
	
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
		if (selectedNodes.size() != 1)
		{
			translationGizmo.hide();
		}
		else
		{
			auto node_itr = scene.nodes.find(*selectedNodes.begin());
			Assert(node_itr != scene.nodes.end());
			
			auto * node = node_itr->second;
			
			// determine the current global transform
			
			Mat4x4 globalTransform(true);
			
			auto * sceneNodeComponent = node->components.find<SceneNodeComponent>();
			Assert(sceneNodeComponent != nullptr);
			
			if (sceneNodeComponent != nullptr)
				globalTransform = sceneNodeComponent->objectToWorld;
			
			// let the gizmo do it's thing
			
			translationGizmo.show(globalTransform);
			
			if (enablePadGizmo)
			{
				enablePadGizmo = false;
				
				if (inputIsCaptured == false)
				{
					translationGizmo.beginPad(cameraPosition, mouseDirection_world);
				}
			}
			
			if (translationGizmo.tick(cameraPosition, mouseDirection_world, inputIsCaptured))
			{
				// transform the global transform into local space
				
				Mat4x4 localTransform = translationGizmo.gizmoToWorld;
				
				if (node->parentId != -1)
				{
					auto parent_itr = scene.nodes.find(node->parentId);
					Assert(parent_itr != scene.nodes.end());
					
					auto * parent = parent_itr->second;
					
					auto * sceneNodeComponent_parent = parent->components.find<SceneNodeComponent>();
					Assert(sceneNodeComponent_parent != nullptr);
				
					if (sceneNodeComponent_parent != nullptr)
						localTransform = sceneNodeComponent_parent->objectToWorld.CalcInv() * localTransform;
				}
				
				// assign the translation in local space to the transform component
				
				auto * transformComponent = node->components.find<TransformComponent>();
				
				if (transformComponent != nullptr)
				{
					transformComponent->position = localTransform.GetTranslation();
					
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
					
				#if ENABLE_QUAT_FIXUP
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
	
		// determine which node is underneath the mouse cursor
		
		const SceneNode * hoverNode = raycast(cameraPosition, mouseDirection_world);
		
		if (inputIsCaptured == false)
		{
			static SDL_Cursor * cursorHand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
			SDL_SetCursor(hoverNode == nullptr ? SDL_GetDefaultCursor() : cursorHand);
		}
		
		if (inputIsCaptured == false && mouse.wentDown(BUTTON_LEFT))
		{
			//inputIsCaptured = true;
			
			if (keyboard.isDown(SDLK_LSHIFT))
			{
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
							std::set<int> nodesToSelect;
							
							for (auto & parentNodeId : selectedNodes)
							{
								auto parentNode_itr = scene.nodes.find(parentNodeId);
								Assert(parentNode_itr != scene.nodes.end());
								if (parentNode_itr != scene.nodes.end())
								{
									auto & parentNode = *parentNode_itr->second;
									
									auto * sceneNodeComp = parentNode.components.find<SceneNodeComponent>();
									
									Assert(sceneNodeComp != nullptr);
									if (sceneNodeComp != nullptr)
									{
										const Vec3 groundPosition_parent = sceneNodeComp->objectToWorld.CalcInv().Mul4(groundPosition);
										
										auto nodeId = addNodeFromTemplate_v2(groundPosition_parent, AngleAxis(), parentNodeId);
										
										// select the newly added node
										nodesToSelect.insert(nodeId);
										nodeToGiveFocus = nodeId;
									}
								}
							}
							
							if (nodesToSelect.empty() == false)
								selectedNodes = nodesToSelect;
						}
						else
						{
							auto nodeId = addNodeFromTemplate_v2(groundPosition, AngleAxis(), scene.rootNodeId);
							
							// select the newly added node
							selectedNodes.clear();
							selectedNodes.insert(nodeId);
							nodeToGiveFocus = nodeId;
							enablePadGizmo = true;
						}
					}
				}
			}
			else
			{
				selectedNodes.clear();
				
				if (hoverNode != nullptr)
				{
					selectedNodes.insert(hoverNode->id);
					nodeToGiveFocus = hoverNode->id;
				}
			}
		}
		
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
		
		if (camera.mode == Camera::kMode_FirstPerson)
			cameraIsActive = mouse.isDown(BUTTON_LEFT);
		else
			cameraIsActive = true;
		
		camera.tick(dt, inputIsCaptured, cameraIsActive == false);
	}
	
	void drawNode(const SceneNode & node) const
	{
		const bool isSelected = selectedNodes.count(node.id) != 0;
		
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
				
				setColor(isSelected ? 255 : 60, 0, 0, 40);
				fillCube(position, size);
			}
		}
	}
	
	void drawNodesTraverse(const SceneNode & node) const
	{
		gxPushMatrix();
		{
			auto transformComp = node.components.find<TransformComponent>();
			
			if (transformComp != nullptr)
			{
				gxTranslatef(
					transformComp->position[0],
					transformComp->position[1],
					transformComp->position[2]);
				
				gxRotatef(
					transformComp->angleAxis.angle,
					transformComp->angleAxis.axis[0],
					transformComp->angleAxis.axis[1],
					transformComp->angleAxis.axis[2]);
				
				gxScalef(
					transformComp->scale,
					transformComp->scale,
					transformComp->scale);
			}
			
			for (auto & childNodeId : node.childNodeIds)
			{
				auto childNodeItr = scene.nodes.find(childNodeId);
				
				Assert(childNodeItr != scene.nodes.end());
				if (childNodeItr != scene.nodes.end())
				{
					const SceneNode & childNode = *childNodeItr->second;
					
					drawNodesTraverse(childNode);
				}
			}
			
			drawNode(node);
		}
		gxPopMatrix();
	}
	
	void drawSceneOpaque() const
	{
		s_modelComponentMgr.draw();
	}
	
	void drawEditorOpaque() const
	{
	#if ENABLE_TRANSFORM_GIZMOS
		translationGizmo.draw();
	#endif
	}
	
	void drawOpaque() const
	{
		pushDepthTest(true, DEPTH_LESS);
		pushBlend(BLEND_OPAQUE);
		{
			if (preview.drawScene)
			{
				drawSceneOpaque();
			}
			
			drawEditorOpaque();
		}
		popBlend();
		popDepthTest();
	}
	
	void drawColors() const
	{
		pushShaderOutputs("c");
		{
			drawOpaque();
		}
		popShaderOutputs();
	}
	
	void drawNormals() const
	{
		pushShaderOutputs("n");
		{
			drawOpaque();
		}
		popShaderOutputs();
	}
	
	void drawSceneTranslucent() const
	{
	}
	
	void drawEditorTranslucent() const
	{
		if (visibility.drawGroundPlane)
		{
			pushLineSmooth(true);
			gxPushMatrix();
			{
				gxScalef(100, 1, 100);
				
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
			drawNodesTraverse(scene.getRootNode());
			popBlend();
		}
	}
	
	void drawTranslucent() const
	{
		pushDepthTest(true, DEPTH_LESS, false);
		pushBlend(BLEND_ALPHA);
		{
			drawSceneTranslucent();
			
			drawEditorTranslucent();
		}
		popBlend();
		popDepthTest();
	}
	
	void drawEditor() const
	{
		camera.pushProjectionMatrix();
		camera.pushViewMatrix();
		{
			if (renderer.mode->get() == Renderer::kMode_Colors)
				drawColors();
			if (renderer.mode->get() == Renderer::kMode_Normals)
				drawNormals();
			
			drawTranslucent();
		}
		camera.popViewMatrix();
		camera.popProjectionMatrix();
		
		projectScreen2d();
		
		const_cast<SceneEditor*>(this)->guiContext.draw();
	}
};

#include "cameraResource.h"

static void testResources()
{
	g_resourceDatabase.add("controller 1", new Resource<CameraControllerTest>());
	g_resourceDatabase.add("controller 2", new Resource<CameraControllerTest>());
}

//

#if 0

#include "vfxgraphComponent.h"

struct ResourcePtrTestComponent : Component<ResourcePtrTestComponent>
{
	ResourcePtr resourcePtr;
	
	virtual void tick(const float dt) override final
	{
		auto * textureResource = resourcePtr.get<TextureResource>();
		
		if (textureResource != nullptr)
			logDebug("texture: %d", textureResource->texture);
	}
};

typedef ComponentMgr<ResourcePtrTestComponent> ResourcePtrTestComponentMgr;

struct ResourcePtrTestComponentType : ComponentType<ResourcePtrTestComponent>
{
	ResourcePtrTestComponentType()
		: ComponentType("ResourcePtrTestComponent")
	{
		in("texture", &ResourcePtrTestComponent::resourcePtr);
	}
};

static bool testResourcePointers()
{
	if (!framework.init(VIEW_SX, VIEW_SY))
		return false;
	
	registerBuiltinTypes();
	registerComponentTypes();
	
	ResourcePtrTestComponentMgr resourcePtrTestComponentMgr;
	registerComponentType(new ResourcePtrTestComponentType(), &resourcePtrTestComponentMgr);
	
	Template t;
	
	if (!loadTemplateFromFile("textfiles/resource-pointer-v1.txt", t))
		logError("failed to load resource pointer test file");
	else
	{
		ComponentSet componentSet;
		
		instantiateComponentsFromTemplate(g_typeDB, t, componentSet);
		
		for (auto * component = componentSet.head; component != nullptr; component = component->next_in_set)
		{
			component->init();
		}
		
		for (int i = 0; i < 10; ++i)
		{
			for (auto * component = componentSet.head; component != nullptr; component = component->next_in_set)
			{
				component->tick(0.f);
			}
		}
		
		freeComponentsInComponentSet(componentSet);
	}
	
	Assert(g_resourceDatabase.head == nullptr);
	
	return true;
}

#endif

static bool node_to_clipboard_text(const SceneNode & node, std::string & text)
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
	
// todo : add more efficient to_string()
	auto lines = line_writer.to_lines();
	
	for (auto & line : lines)
	{
		text += line;
		text += '\n';
	}
	
	return true;
}

static bool node_from_clipboard_text(const char * text, SceneNode & node)
{
	std::vector<std::string> lines;
	TextIO::LineEndings lineEndings;
	
	if (TextIO::loadText(text, lines, lineEndings) == false)
		return false;
	
	LineReader line_reader(lines, 0, 0);
	
	const char * id = line_reader.get_next_line(true);
	
	if (strcmp(id, ":node") != 0)
	{
		return false;
	}
	
	for (;;)
	{
		const char * component_type_name = line_reader.get_next_line(true);
		
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

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#else
	changeDirectory(SDL_GetBasePath());
#endif

	framework.enableDepthBuffer = true;
	framework.allowHighDpi = false;

#if 0
	test_templates();
	return 0;
#endif

#if 0
	test_scenefiles();
	return 0;
#endif

#if 0
	if (!test_templateEditor())
		logError("failure!");
	return 0;
#endif

#if 0
	testResourcePointers();
	return 0;
#endif

#if 0
	test_reflection_1();
	return 0;
#endif

#if 0
	test_bindObjectToFile();
	return 0;
#endif
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	registerBuiltinTypes();
	registerComponentTypes();
	initComponentMgrs();
	
	testResources(); // todo : remove
	
	SceneEditor editor;
	editor.init();
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;

		const float dt = framework.timeStep;
		
		bool inputIsCaptured = false;
		
		editor.tickEditor(dt, inputIsCaptured);
		
	#if ENABLE_COMPONENT_JSON
		// save load test. todo : remove test code
		
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_s))
		{
			inputIsCaptured = true;
			
			auto t1 = SDL_GetTicks();
			
			if (!keyboard.isDown(SDLK_LSHIFT))
			{
				if (editor.scene.saveToFile("testScene.json") == false)
					logError("failed to save scene to json");
				else
				{
				#if 1
					Scene tempScene;
					
					if (tempScene.loadFromFile("testScene.json") == false)
						logError("failed to load scene from json");
					else
					{
						editor.deferredBegin();
						{
							for (auto & node_itr : editor.scene.nodes)
								editor.deferred.nodesToRemove.insert(node_itr.second->id);
						}
						editor.deferredEnd();
						
						editor.scene = tempScene;
					}
				#endif
				}
			}
			
			LineWriter line_writer;
			if (editor.scene.saveToLines(g_typeDB, line_writer) == false)
				logError("failed to save scene to lines");
			else
			{
				auto lines = line_writer.to_lines();
		
				if (TextIO::save("testScene.txt", lines, TextIO::kLineEndings_Unix) == false)
					logError("failed to save lines to file");
				else
				{
					// todo : load it
				}
			}
			
			auto t2 = SDL_GetTicks();
			printf("time: %ums\n", (t2 - t1));
			printf("(done)\n");
		}
		
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_l))
		{
			inputIsCaptured = true;
			
			Scene tempScene;
			
			if (tempScene.loadFromFile("testScene.json"))
			{
				editor.deferredBegin();
				{
					for (auto & node_itr : editor.scene.nodes)
						editor.deferred.nodesToRemove.insert(node_itr.second->id);
				}
				editor.deferredEnd();
				
				editor.scene = tempScene;
			}
		}
	#endif
	
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_t))
		{
			inputIsCaptured = true;
			
			// load scene description text file
	
			changeDirectory("textfiles"); // todo : use a nicer solution to handling relative paths
			
			std::vector<std::string> lines;
			TextIO::LineEndings lineEndings;
			
			if (!TextIO::load("scene-v1.txt", lines, lineEndings))
			{
				logError("failed to load text file");
			}
			else
			{
				Scene tempScene;
				tempScene.createRootNode();

				if (!parseSceneFromLines(g_typeDB, lines, tempScene))
				{
					logError("failed to parse scene from lines");
				}
				else
				{
					editor.deferredBegin();
					{
						for (auto & node_itr : editor.scene.nodes)
							editor.deferred.nodesToRemove.insert(node_itr.second->id);
					}
					editor.deferredEnd();
					Assert(editor.scene.nodes.empty());
					
					editor.scene = tempScene;
				}
			}
			
			changeDirectory(".."); // fixme : remove
		}
		
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_p))
		{
			inputIsCaptured = true;
			
			auto & rootNode = editor.scene.getRootNode();
			
			if (!rootNode.childNodeIds.empty())
			{
				auto nodeId = rootNode.childNodeIds[0];
				
				auto node_itr = editor.scene.nodes.find(nodeId);
				Assert(node_itr != editor.scene.nodes.end());
				{
					auto * node = node_itr->second;
					
					if (node->components.head != nullptr)
					{
						auto * component = node->components.head;
						auto * componentType = findComponentType(component->typeIndex());
						
						Assert(componentType != nullptr);
						if (componentType != nullptr)
						{
							auto t1 = SDL_GetTicks();
							for (int i = 0; i < 100000; ++i)
							{
						#if 1
							LineWriter line_writer;
							object_tolines_recursive(g_typeDB, componentType, component, line_writer, 0);
							
							std::vector<std::string> lines = line_writer.to_lines();
							
							//for (auto & line : lines)
							//	logInfo("%s", line.c_str());
							
							auto * component_copy = componentType->componentMgr->createComponent(nullptr);
							
							LineReader line_reader(lines, 0, 0);
							if (object_fromlines_recursive(g_typeDB, componentType, component_copy, line_reader))
							{
								//logDebug("success!");
							}
							
							componentType->componentMgr->destroyComponent(component_copy);
						#else
							nlohmann::json json;
							ComponentJson j1(json);
							
							member_tojson_recursive(g_typeDB, componentType, component, j1);
							
							auto str = json.dump(4);
							//printf("%s", str.c_str());
							
							json = json.parse(str);
							ComponentJson j2(json);
							
							auto * component_copy = componentType->componentMgr->createComponent(nullptr);
							
							if (member_fromjson_recursive(g_typeDB, componentType, component_copy, j2))
							{
								//logDebug("success!");
							}
							
							componentType->componentMgr->destroyComponent(component_copy);
						#endif
							}
							auto t2 = SDL_GetTicks();
							printf("time: %ums\n", (t2 - t1));
							printf("(done)\n");
						}
					}
				}
			}
		}
		
		//
		
		s_transformComponentMgr.calculateTransforms(editor.scene);
		
		if (editor.preview.tickScene)
		{
			const float dt_scene = dt * editor.preview.tickMultiplier;
			
			for (auto * type : g_componentTypes)
			{
				type->componentMgr->tick(dt_scene);
			}
			
			s_transformComponentMgr.calculateTransforms(editor.scene);
		}
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			editor.drawEditor();
		}
		framework.endDraw();
	}
	
	editor.shut();
	
	shutComponentMgrs();
	
	framework.shutdown();

	return 0;
}
