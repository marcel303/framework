#define DEFINE_COMPONENT_TYPES
#include "modelComponent.h"
#include "transformComponent.h"

#include "component.h"
#include "componentType.h"
#include "framework.h"
#include "imgui-framework.h"
#include "Parse.h"
#include "Quat.h"
#include "StringEx.h"

#include <algorithm>
#include <map>
#include <set>
#include <typeindex>

static const int VIEW_SX = 800;
static const int VIEW_SY = 600;

//

struct ComponentTypeRegistration
{
	ComponentMgrBase * componentMgr = nullptr;
	ComponentTypeBase * componentType = nullptr;
};

std::vector<ComponentTypeRegistration> s_componentTypeRegistrations;

void registerComponentType(ComponentTypeBase * componentType, ComponentMgrBase * componentMgr)
{
	ComponentTypeRegistration registration;
	registration.componentMgr = componentMgr;
	registration.componentType = componentType;

	s_componentTypeRegistrations.push_back(registration);
	
	std::sort(s_componentTypeRegistrations.begin(), s_componentTypeRegistrations.end(), [](const ComponentTypeRegistration & r1, const ComponentTypeRegistration & r2) { return r1.componentType->tickPriority < r2.componentType->tickPriority; });
}

static TransformComponentMgr s_transformComponentMgr;
static ModelComponentMgr s_modelComponentMgr;

struct SceneNode
{
	int id = -1;
	int parentId = -1;
	std::string displayName;
	
	std::vector<int> childNodeIds;
	
	std::vector<ComponentBase*> components;
	
	template <typename T>
	T * findComponent()
	{
		for (auto * component : components)
		{
			if (component->typeIndex() == std::type_index(typeid(T)))
				return static_cast<T*>(component);
		}
		
		return nullptr;
	}
	
	template <typename T>
	const T * findComponent() const
	{
		for (auto * component : components)
		{
			if (component->typeIndex() == std::type_index(typeid(T)))
				return static_cast<T*>(component);
		}
		
		return nullptr;
	}
	
	void freeComponents()
	{
		for (auto * component : components)
		{
		// todo : find the appropriate component mgr from registration list
			if (component->typeIndex() == s_modelComponentMgr.typeIndex())
				s_modelComponentMgr.removeComponentForNode(id, s_modelComponentMgr.castToComponentType(component));
		}
		
		components.clear();
	}
};

struct Scene
{
	std::map<int, SceneNode> nodes;
	
	int nextNodeId = 0;
	
	int rootNodeId = -1;
	
	Scene()
	{
		SceneNode rootNode;
		rootNode.id = allocNodeId();
		rootNode.displayName = "root";
		
		nodes[rootNode.id] = rootNode;
		
		rootNodeId = rootNode.id;
	}
	
	int allocNodeId()
	{
		return nextNodeId++;
	}
	
	SceneNode & getRootNode()
	{
		auto i = nodes.find(rootNodeId);
		
		return i->second;
	}
	
	const SceneNode & getRootNode() const
	{
		auto i = nodes.find(rootNodeId);
		
		return i->second;
	}
};

// todo : move to transform component source file

void TransformComponentMgr::calculateTransformsTraverse(Scene & scene, SceneNode & node) const
{
	gxPushMatrix();
	{
		auto transformComp = node.findComponent<TransformComponent>();
		
		if (transformComp != nullptr)
		{
			gxTranslatef(
				transformComp->position[0],
				transformComp->position[1],
				transformComp->position[2]);
			
			gxRotatef(
				transformComp->angleAxis.angle * 180.f / float(M_PI),
				transformComp->angleAxis.axis[0],
				transformComp->angleAxis.axis[1],
				transformComp->angleAxis.axis[2]);
			
			gxScalef(
				transformComp->scale,
				transformComp->scale,
				transformComp->scale);
			
			gxGetMatrixf(GX_MODELVIEW, transformComp->globalTransform.m_v);
		}
		
		auto modelComp = node.findComponent<ModelComponent>();
		
		if (modelComp != nullptr)
		{
			gxGetMatrixf(GX_MODELVIEW, modelComp->objectToWorld.m_v);
		}
		
		for (auto & childNodeId : node.childNodeIds)
		{
			auto childNodeItr = scene.nodes.find(childNodeId);
			
			Assert(childNodeItr != scene.nodes.end());
			if (childNodeItr != scene.nodes.end())
			{
				SceneNode & childNode = childNodeItr->second;
				
				calculateTransformsTraverse(scene, childNode);
			}
		}
	}
	gxPopMatrix();
}

void TransformComponentMgr::calculateTransforms(Scene & scene) const
{
	calculateTransformsTraverse(scene, scene.getRootNode());
}

//

struct SceneEditor
{
	enum State
	{
		kState_Idle,
		kState_NodeDrag
	};
	
	Camera3d camera;
	
	Scene scene;
	
	std::set<int> selectedNodes;
	
	bool drawGroundPlane = true;
	bool drawNodes = true;
	bool drawNodeBoundingBoxes = true;
	
	FrameworkImGuiContext guiContext;
	
	std::set<int> nodesToRemove;
	
	bool cameraIsActive = false;
	
	SceneEditor()
	{
		camera.position = Vec3(0, 1, -2);
	}
	
	void init()
	{
		guiContext.init();
	}
	
	void shut()
	{
		guiContext.shut();
	}
	
	SceneNode * raycast(Vec3Arg rayOrigin, Vec3Arg rayDirection)
	{
	#if 0
	// todo : traverse node hierarchy and apply transforms
		SceneNode * result = nullptr;
		float bestDistance = 0.f;
		
		for (auto & nodeItr : scene.nodes)
		{
			auto & node = nodeItr.second;
			
			const float distance = (node.position - rayOrigin).CalcSize();
			
			if (distance < bestDistance || result == nullptr)
			{
				result = &node;
				bestDistance = distance;
			}
		}
		
		return result;
	#else
		if (scene.nodes.empty())
			return nullptr;
		else
		{
			int index = rand() % scene.nodes.size();
			auto i = scene.nodes.begin();
			while (index-- > 0)
				i++;
			return &i->second;
		}
	#endif
	}
	
	void removeNodeTraverse(const int nodeId)
	{
		auto nodeItr = scene.nodes.find(nodeId);
		
		Assert(nodeItr != scene.nodes.end());
		if (nodeItr != scene.nodes.end())
		{
			auto & node = nodeItr->second;
			
			if (node.parentId != -1)
			{
				auto parentNodeItr = scene.nodes.find(node.parentId);
				Assert(parentNodeItr != scene.nodes.end());
				if (parentNodeItr != scene.nodes.end())
				{
					auto & parentNode = parentNodeItr->second;
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
			
			scene.nodes.erase(nodeItr);
			
			selectedNodes.erase(nodeId);
			
			nodesToRemove.erase(nodeId);
		}
	}
	
	void removeNodesToRemove()
	{
		// todo : remove nodesToRemove and directly call removeNodeTraverse ?
		
		while (!nodesToRemove.empty())
		{
			auto nodeToRemoveItr = nodesToRemove.begin();
			auto nodeId = *nodeToRemoveItr;
			
			removeNodeTraverse(nodeId);
		}

		Assert(nodesToRemove.empty());
	#if defined(DEBUG)
		for (auto & selectedNodeId : selectedNodes)
			Assert(scene.nodes.find(selectedNodeId) != scene.nodes.end());
	#endif
	}
	
	void editNodeListTraverse(const int nodeId)
	{
		auto nodeItr = scene.nodes.find(nodeId);
		Assert(nodeItr != scene.nodes.end());
		if (nodeItr == scene.nodes.end())
			return;
		
		SceneNode & node = nodeItr->second;
		
		ImGui::PushID(nodeId);
		{
			if (ImGui::BeginPopupContextItem("NodeMenu"))
			{
				//logDebug("context window for %d", nodeId);
				
				if (ImGui::MenuItem("Remove"))
				{
					nodesToRemove.insert(nodeId);
				}
				
				if (ImGui::MenuItem("Insert node"))
				{
					SceneNode childNode;
					childNode.id = scene.allocNodeId();
					childNode.parentId = nodeId;
					childNode.displayName = String::FormatC("Node %d", childNode.id);
					scene.nodes[childNode.id] = childNode;
					
					scene.nodes[nodeId].childNodeIds.push_back(childNode.id);
				}
				
				for (auto & r : s_componentTypeRegistrations)
				{
					bool isAdded = false;
					
					for (auto * component : node.components)
						if (component->typeIndex() == r.componentMgr->typeIndex())
							isAdded = true;
					
					if (isAdded == false)
					{
						char text[256];
						sprintf_s(text, sizeof(text), "Add %s", r.componentType->typeName.c_str());
						
						if (ImGui::MenuItem(text))
						{
							auto component = r.componentMgr->createComponentForNode(node.id);
							node.components.push_back(component);
						}
					}
				}

				ImGui::EndPopup();
			}
			
			ImGui::Indent();
			{
				for (auto * component : node.components)
				{
					ImGui::PushID(component);
					{
						ComponentTypeRegistration * r = nullptr;
						
						for (auto & registration : s_componentTypeRegistrations)
							if (registration.componentMgr->typeIndex() == component->typeIndex())
								r = &registration;
						
						Assert(r != nullptr);
						if (r != nullptr)
						{
							auto & type = r->componentType;
							
							ImGui::LabelText("Component", "%s", type->typeName.c_str());
							
							for (auto & propertyBase : type->properties)
							{
								switch (propertyBase->type)
								{
								case kComponentPropertyType_Int32:
									{
									}
									break;
								case kComponentPropertyType_Float:
									{
										auto property = static_cast<ComponentPropertyFloat*>(propertyBase);
										
										auto & value = property->getter(component);
										
										if (property->hasLimits)
											ImGui::SliderFloat(property->name.c_str(), &value, property->min, property->max, "%.3f", property->editingCurveExponential);
										else
											ImGui::InputFloat(property->name.c_str(), &value);
									}
									break;
								case kComponentPropertyType_Vec2:
									{
										auto property = static_cast<ComponentPropertyVec2*>(propertyBase);
										
										auto & value = property->getter(component);
							
										ImGui::InputFloat2(property->name.c_str(), &value[0]);
									}
									break;
								case kComponentPropertyType_Vec3:
									{
										auto property = static_cast<ComponentPropertyVec3*>(propertyBase);
										
										auto & value = property->getter(component);
							
										ImGui::InputFloat3(property->name.c_str(), &value[0]);
									}
									break;
								case kComponentPropertyType_Vec4:
									{
										auto property = static_cast<ComponentPropertyVec4*>(propertyBase);
										
										auto & value = property->getter(component);
							
										ImGui::InputFloat4(property->name.c_str(), &value[0]);
									}
									break;
								case kComponentPropertyType_String:
									{
										auto property = static_cast<ComponentPropertyString*>(propertyBase);
										
										auto & value = property->getter(component);
							
										char buffer[1024];
										strcpy_s(buffer, sizeof(buffer), value.c_str());
										
										if (ImGui::InputText(property->name.c_str(), buffer, sizeof(buffer)))
											property->setter(component, buffer);
									}
									break;
								case kComponentPropertyType_AngleAxis:
									{
										auto property = static_cast<ComponentPropertyAngleAxis*>(propertyBase);
										
										auto & value = property->getter(component);
										
										// todo : also edit axis
										ImGui::SliderAngle(property->name.c_str(), &value.angle);
									}
									break;
								}
							}
						}
					}
					ImGui::PopID();
				}
			
				for (auto & childNodeId : node.childNodeIds)
				{
					auto childNodeItr = scene.nodes.find(childNodeId);
					
					Assert(childNodeItr != scene.nodes.end());
					if (childNodeItr != scene.nodes.end())
					{
						ImGui::PushID(childNodeId);
						
						auto & childNode = childNodeItr->second;
						
						if (ImGui::CollapsingHeader(childNode.displayName.c_str()))
						{
							editNodeListTraverse(childNodeId);
						}
				
						ImGui::PopID();
					}
				}
			}
			ImGui::Unindent();
		}
		ImGui::PopID();
	}
	
	void tickEditor(const float dt, bool & inputIsCaptured)
	{
		if (cameraIsActive == false)
		{
			guiContext.processBegin(dt, 800, 600, inputIsCaptured);
			{
				if (ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Checkbox("Draw ground plane", &drawGroundPlane);
					ImGui::Checkbox("Draw nodes", &drawNodes);
					ImGui::Checkbox("Draw node bounding boxes", &drawNodeBoundingBoxes);
					
					ImGui::Text("Root node");
					editNodeListTraverse(scene.rootNodeId);
					
					removeNodesToRemove();
				}
				ImGui::End();
			}
			guiContext.processEnd();
		}
		
		//if (inputIsCaptured == false)
		//if (keyboard.wentDown(SDLK_s))
		{
			//inputIsCaptured = true;
			
			const SceneNode * node = raycast(camera.position, camera.getWorldMatrix().GetAxis(2));
			
			selectedNodes.clear();
			
			if (node != nullptr)
				selectedNodes.insert(node->id);
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
		
		if (inputIsCaptured == false)
		{
			if (mouse.wentDown(BUTTON_LEFT))
				cameraIsActive = true;
		}
		
		if (cameraIsActive)
		{
			if (inputIsCaptured || mouse.wentUp(BUTTON_LEFT))
				cameraIsActive = false;
		}
		
		camera.tick(dt, cameraIsActive);
	}
	
	void drawNode(const SceneNode & node) const
	{
		const bool isSelected = selectedNodes.count(node.id) != 0;
		
		setColor(isSelected ? colorYellow : colorWhite);
		fillCube(Vec3(), Vec3(.1f, .1f, .1f));
		
		const ModelComponent * modelComp = node.findComponent<ModelComponent>();
		
		if (modelComp != nullptr)
		{
			const Vec3 & min = modelComp->aabbMin;
			const Vec3 & max = modelComp->aabbMax;
			
			const Vec3 position = (min + max) / 2.f;
			const Vec3 size = (max - min) / 2.f;
			
			setColor(255, 0, 0, 40);
			fillCube(position, size);
		}
	}
	
	void drawNodesTraverse(const SceneNode & node) const
	{
		gxPushMatrix();
		{
			auto transformComp = node.findComponent<TransformComponent>();
			
			if (transformComp != nullptr)
			{
				gxTranslatef(
					transformComp->position[0],
					transformComp->position[1],
					transformComp->position[2]);
				
				gxRotatef(
					transformComp->angleAxis.angle * 180.f / float(M_PI),
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
					const SceneNode & childNode = childNodeItr->second;
					
					drawNodesTraverse(childNode);
				}
			}
			
			drawNode(node);
		}
		gxPopMatrix();
	}
	
	void drawEditor() const
	{
		projectPerspective3d(60.f, .1f, 100.f);
		camera.pushViewMatrix();
		{
			if (drawGroundPlane)
			{
				pushLineSmooth(true);
				pushDepthTest(true, DEPTH_LESS);
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
			
			if (true)
			{
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_OPAQUE);
				s_modelComponentMgr.draw();
				popBlend();
				popDepthTest();
			}
			
			if (drawNodes)
			{
				pushDepthWrite(false);
				pushBlend(BLEND_ADD);
				drawNodesTraverse(scene.getRootNode());
				popBlend();
				popDepthWrite();
			}
		}
		camera.popViewMatrix();
		
		projectScreen2d();
		
		const_cast<SceneEditor*>(this)->guiContext.draw();
	}
};

static void createRandomScene(Scene & scene)
{
	auto & rootNode = scene.getRootNode();
	
	for (int i = 0; i < 8; ++i)
	{
		SceneNode node;
		
		node.id = scene.allocNodeId();
		node.parentId = scene.rootNodeId;
		node.displayName = String::FormatC("Node %d", node.id);
		
		auto modelComp = s_modelComponentMgr.createComponentForNode(node.id);
		
		if (modelComp->init({{ "filename", "model.txt" } }))
		{
			node.components.push_back(modelComp);
		}
		
		auto transformComp = s_transformComponentMgr.createComponentForNode(node.id);
		
		if (transformComp->init({ }))
		{
			transformComp->position[0] = random(-4.f, +4.f);
			transformComp->position[1] = random(-4.f, +4.f);
			transformComp->position[2] = random(-4.f, +4.f);
			transformComp->scale = .01f;
			
			node.components.push_back(transformComp);
		}
		
		scene.nodes[node.id] = node;
		
		rootNode.childNodeIds.push_back(node.id);
	}
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif

	framework.enableDepthBuffer = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	registerComponentType(new TransformComponentType(), &s_transformComponentMgr);
	registerComponentType(new ModelComponentType(), &s_modelComponentMgr);
	
	SceneEditor editor;
	editor.init();
	
	createRandomScene(editor.scene);
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		const float dt = framework.timeStep;
		
		bool inputIsCaptured = false;
		
		editor.tickEditor(dt, inputIsCaptured);
		
		s_transformComponentMgr.calculateTransforms(editor.scene);
		
		for (auto & r : s_componentTypeRegistrations)
		{
			r.componentMgr->tick(dt);
		}
		
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
