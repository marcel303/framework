#include "cameraComponent.h"
#include "modelComponent.h"
#include "parameterComponent.h"
#include "parameterComponentUi.h"
#include "transformComponent.h"

#include "component.h"
#include "componentPropertyUi.h"
#include "componentType.h"
#include "framework.h"
#include "framework-camera.h"
#include "imgui-framework.h"
#include "scene.h"
#include "scene_fromText.h"
#include "sceneNodeComponent.h"
#include "StringEx.h"
#include "TextIO.h"

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

/*

todo :

- add libreflection-jsonio
- remove json references from scene edit
- add general monitor camera. or extend framework's Camera3d to support perspective, orbit and ortho modes
- avoid UI from jumping around
	- add independent scrolling area for scene structure
	- add independent scrolling area for selected node
- use scene structure tree to select nodes. remove inline editing
- add lookat/focus option to scene structure view
	- as a context menu
- separate node traversal/scene structure view from node editor

*/

extern void test_templates();
extern bool test_scenefiles();
extern bool test_templateEditor();
extern void test_reflection_1();
extern void test_bindObjectToFile();

static const int VIEW_SX = 1200;
static const int VIEW_SY = 800;

// todo : move these instances to helpers.cpp
TransformComponentMgr s_transformComponentMgr;
ModelComponentMgr s_modelComponentMgr;
extern ParameterComponentMgr s_parameterComponentMgr;

// todo : move to helpers
static bool node_to_clipboard_text(const SceneNode & node, std::string & text);
static bool node_from_clipboard_text(const char * text, SceneNode & node);

//

/**
 * Intersects a bounding box given by (min, max) with a ray specified by its origin and direction. If there is an intersection, the function returns true, and stores the distance of the point of intersection in 't'. If there is no intersection, the function returns false and leaves 't' unmodified. Note that the ray direction is expected to be the inverse of the actual ray direction, for efficiency reasons.
 * @param min: Minimum of bounding box extents.
 * @param max: Maximum of bounding box extents.
 * @param px: Origin X of ray.
 * @param py: Origin Y of ray.
 * @param pz: Origin Z of ray.
 * @param dxInv: Inverse of direction X of ray.
 * @param dyInv: Inverse of direction Y of ray.
 * @param dzInv: Inverse of direction Z of ray.
 * @param t: Stores the distance to the intersection point if there is an intersection.
 * @return: True if there is an intersection. False otherwise.
 */
static bool intersectBoundingBox3d(const float * min, const float * max, const float px, const float py, const float pz, const float dxInv, const float dyInv, const float dzInv, float & t)
{
	float tmin = std::numeric_limits<float>().min();
	float tmax = std::numeric_limits<float>().max();

	const float p[3] = { px, py, pz };
	const float rd[3] = { dxInv, dyInv, dzInv };

	for (int i = 0; i < 3; ++i)
	{
		const float t1 = (min[i] - p[i]) * rd[i];
		const float t2 = (max[i] - p[i]) * rd[i];

		tmin = std::max(tmin, std::min(t1, t2));
		tmax = std::min(tmax, std::max(t1, t2));
	}

	if (tmax >= tmin)
	{
		t = tmin;

		return true;
	}
	else
	{
		return false;
	}
}

#if 0

#include "SIMD.h" // todo : create SimdVec.h, add SimdBoundingBox.h

static bool intersectBoundingBox3d_simd(SimdVecArg rayOrigin, SimdVecArg rayDirectionInv, SimdVecArg boxMin, SimdVecArg boxMax, /*SimdVec & ioDistance*/float & ioDistance)
{
	const static SimdVec infNeg(-std::numeric_limits<float>::infinity());
	const static SimdVec infPos(+std::numeric_limits<float>::infinity());

	// distances
	SimdVec minVecTemp = boxMin.Sub(rayOrigin).Mul(rayDirectionInv);
	SimdVec maxVecTemp = boxMax.Sub(rayOrigin).Mul(rayDirectionInv);

	SimdVec minVec = minVecTemp.Min(maxVecTemp);
	SimdVec maxVec = minVecTemp.Max(maxVecTemp);

	// calculcate distance
	const SimdVec max =                    maxVec.ReplicateX().Min(maxVec.ReplicateY().Min(maxVec.ReplicateZ()));
	const SimdVec min = SimdVec(VZERO).Max(minVec.ReplicateX().Max(minVec.ReplicateY().Max(minVec.ReplicateZ())));

	// no intersection or greater distance?
	if (min.ANY_GE3(max))
		return false;

	ioDistance = min.X();

	return true;
}

#endif

//

struct SceneEditor
{
	enum State
	{
		kState_Idle,
		kState_NodeDrag
	};
	
	Camera camera;
	
	Scene scene;
	
	std::set<int> selectedNodes;
	
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
	
	std::set<int> nodesToRemove;
	std::vector<SceneNode*> nodesToAdd;
	
	bool cameraIsActive = false;
	
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
		
		guiContext.pushImGuiContext();
		ImGui::StyleColorsClassic();
		guiContext.popImGuiContext();
		
		scene.createRootNode();
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
			
			nodesToRemove.erase(nodeId);
		}
	}
	
	void addNodesToAdd()
	{
		for (SceneNode * node : nodesToAdd)
		{
			scene.nodes[node->id] = node;
			
			auto parentNode_itr = scene.nodes.find(node->parentId);
			Assert(parentNode_itr != scene.nodes.end());
			
			auto * parentNode = parentNode_itr->second;
			parentNode->childNodeIds.push_back(node->id);
		}
		
		nodesToAdd.clear();
	}
	
	void removeNodesToRemove()
	{
		while (!nodesToRemove.empty())
		{
			auto nodeToRemoveItr = nodesToRemove.begin();
			auto nodeId = *nodeToRemoveItr;
			
			removeNodeTraverse(nodeId, true);
		}

		Assert(nodesToRemove.empty());
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
		
		auto pasteNodeFromClipboard = [&](const int parentId)
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
					nodesToAdd.push_back(childNode);
					
					// select the newly added child node
					selectedNodes.clear();
					selectedNodes.insert(childNode->id);
				}
			}
			
			SDL_free((void*)text);
			text = nullptr;
		};
		
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
			
			nodesToRemove.insert(node.id);
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
				nodesToAdd.push_back(childNode);
				
				// select the newly added child node
				selectedNodes.clear();
				selectedNodes.insert(childNode->id);
			}
			
			if (nodeUi.nodeDisplayNameFilter[0] != 0)
			{
				// when a display name filter is set the node would just disappear. set the filter to the name of the newly
				// added node in this case, to avoid confusion
				strcpy_s(nodeUi.nodeDisplayNameFilter, sizeof(nodeUi.nodeDisplayNameFilter), childNode->displayName.c_str());
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
		
		//if (mouse.isCaptured == false)
		{
			guiContext.processBegin(dt, VIEW_SX, VIEW_SY, inputIsCaptured);
			{
				ImGui::SetNextWindowPos(ImVec2(4, 4));
				ImGui::SetNextWindowSize(ImVec2(370, VIEW_SY - 8));
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
							
							editNodeStructure_traverse(scene.rootNodeId);
							
							nodeUi.visibleNodes.clear();
							nodeUi.openNodes.clear();
						}
						ImGui::EndChild();
					}
					
					addNodesToAdd();
					
					if (ImGui::CollapsingHeader("Selected node(s)", ImGuiTreeNodeFlags_DefaultOpen))
					{
						ImGui::BeginChild("Selected nodes", ImVec2(0, 300), ImGuiWindowFlags_AlwaysVerticalScrollbar);
						{
							for (auto & selectedNodeId : selectedNodes)
							{
								editNode(selectedNodeId);
							}
						}
						ImGui::EndChild();
					}
					
					removeNodesToRemove();
				}
				ImGui::End();
				
				//
				
			#if 0 // todo : re-enable parameter UI
				if (ImGui::Begin("Parameter UI"))
				{
					ImGui::InputText("Component filter", parameterUi.component_filter, kMaxParameterFilter);
					ImGui::InputText("Parameter filter", parameterUi.parameter_filter, kMaxParameterFilter);
					ImGui::Checkbox("Show components with empty prefix", &parameterUi.showAnonymousComponents);
					
					doParameterUi(s_parameterComponentMgr, parameterUi.component_filter, parameterUi.parameter_filter, parameterUi.showAnonymousComponents);
				}
				ImGui::End();
			#endif
			}
			guiContext.processEnd();
		}
		
		// transform mouse coordinates into a world space direction vector
	
		Mat4x4 cameraToWorld;
		camera.calculateWorldMatrix(cameraToWorld);
		
		int viewportSx;
		int viewportSy;
		framework.getCurrentViewportSize(viewportSx, viewportSy);
		
		Mat4x4 projectionMatrix;
		camera.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);
		
		const Vec3 cameraPosition = cameraToWorld.GetTranslation();
		
		const Vec2 mousePosition_screen(
			mouse.x,
			mouse.y);
		const Vec2 mousePosition_clip(
			mousePosition_screen[0] / float(VIEW_SX) * 2.f - 1.f,
			mousePosition_screen[1] / float(VIEW_SY) * 2.f - 1.f);
		const Vec2 mousePosition_view = projectionMatrix.CalcInv().Mul(mousePosition_clip);
		const Vec3 mouseDirection_world = cameraToWorld.Mul3(
			Vec3(
				+mousePosition_view[0],
				-mousePosition_view[1],
				1.f));
		
		// determine which node is underneath the mouse cursor
		
		const SceneNode * hoverNode = raycast(cameraPosition, mouseDirection_world);
		
		static SDL_Cursor * cursorHand = SDL_CreateSystemCursor(SDL_SYSTEM_CURSOR_HAND);
		SDL_SetCursor(hoverNode == nullptr ? SDL_GetDefaultCursor() : cursorHand);
		
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
						}
					}
				}
			}
			else
			{
				selectedNodes.clear();
				
				if (hoverNode != nullptr)
					selectedNodes.insert(hoverNode->id);
			}
		}
		
		if (inputIsCaptured == false && (keyboard.wentDown(SDLK_BACKSPACE) || keyboard.wentDown(SDLK_DELETE)))
		{
			inputIsCaptured = true;
			
			for (auto nodeId : selectedNodes)
			{
				if (nodeId == scene.rootNodeId)
					continue;
				
				nodesToRemove.insert(nodeId);
			}
			
			removeNodesToRemove();
		}
		
		if (camera.mode == Camera::kMode_FirstPerson)
		{
			if (inputIsCaptured == false)
			{
				if (mouse.wentDown(BUTTON_LEFT))
					cameraIsActive = true;
			}
			
			if (cameraIsActive)
			{
				if (mouse.wentUp(BUTTON_LEFT))
					cameraIsActive = false;
			}
		}
		else
		{
			cameraIsActive = true;
		}
		
		camera.tick(dt, inputIsCaptured, cameraIsActive == false);
	}
	
	void drawNode(const SceneNode & node) const
	{
		const bool isSelected = selectedNodes.count(node.id) != 0;
		
		setColor(isSelected ? colorYellow : colorWhite);
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
	
	void drawEditor() const
	{
		camera.pushProjectionMatrix();
		camera.pushViewMatrix();
		{
			if (preview.drawScene)
			{
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_OPAQUE);
				s_modelComponentMgr.draw();
				popBlend();
				popDepthTest();
			}
			
			if (visibility.drawGroundPlane)
			{
				pushLineSmooth(true);
				pushDepthTest(true, DEPTH_LESS, false);
				pushBlend(BLEND_ALPHA);
				gxPushMatrix();
				{
					gxScalef(100, 1, 100);
					
					setLumi(200);
					drawGrid3dLine(100, 100, 0, 2, true);
					
					setLumi(50);
					drawGrid3dLine(500, 500, 0, 2, true);
				}
				gxPopMatrix();
				popBlend();
				popDepthTest();
				popLineSmooth();
			}
			
			if (visibility.drawNodes)
			{
				pushDepthWrite(false);
				pushBlend(BLEND_ADD);
				drawNodesTraverse(scene.getRootNode());
				popBlend();
				popDepthWrite();
			}
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
						for (auto & node_itr : editor.scene.nodes)
							editor.nodesToRemove.insert(node_itr.second->id);
						editor.removeNodesToRemove();
						
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
				for (auto & node_itr : editor.scene.nodes)
					editor.nodesToRemove.insert(node_itr.second->id);
				editor.removeNodesToRemove();
				
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
					for (auto & node_itr : editor.scene.nodes)
						editor.nodesToRemove.insert(node_itr.second->id);
					editor.removeNodesToRemove();
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
	
	framework.shutdown();

	return 0;
}
