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

struct TransformComponent : Component<TransformComponent>
{
	Vec3 position;
	AngleAxis angleAxis;
	float scale = 1.f;
	
	Mat4x4 globalTransform = Mat4x4(true);
	
	virtual bool init(const std::vector<KeyValuePair> & params) override
	{
		for (auto & param : params)
		{
			if (strcmp(param.key, "scale") == 0)
				scale = Parse::Float(param.value);
			else if (strcmp(param.key, "x") == 0)
				position[0] = Parse::Float(param.value);
			else if (strcmp(param.key, "y") == 0)
				position[1] = Parse::Float(param.value);
			else if (strcmp(param.key, "z") == 0)
				position[2] = Parse::Float(param.value);
		}
		
		return true;
	}
};

struct TransformComponentType : ComponentType<TransformComponent>
{
	TransformComponentType()
	{
		typeName = "TransformComponent";
		
		in("position", &TransformComponent::position);
		in("angleAxis", &TransformComponent::angleAxis);
		in("scale", &TransformComponent::scale)
			.setLimits(0.f, 10.f)
			.setEditingCurveExponential(2.f);
	}
};

struct Scene;
struct SceneNode;

struct TransformComponentMgr : ComponentMgr<TransformComponent>
{
	void calculateTransformsTraverse(Scene & scene, SceneNode & node) const;
	void calculateTransforms(Scene & scene) const;
};

//

struct ModelComponent : Component<ModelComponent>
{
	std::string filename;
	
	Vec3 aabbMin;
	Vec3 aabbMax;
	
	Mat4x4 objectToWorld = Mat4x4(true);
	
	virtual bool init(const std::vector<KeyValuePair> & params) override
	{
		for (auto & param : params)
		{
			if (strcmp(param.key, "filename") == 0)
				filename = param.value;
		}
		
		Model(filename.c_str()).calculateAABB(aabbMin, aabbMax, true);
		
		return true;
	}
	
	void draw() const
	{
		if (filename.empty())
			return;
		
		gxPushMatrix();
		{
			gxMultMatrixf(objectToWorld.m_v);
			
			setColor(colorWhite);
			Model(filename.c_str()).draw();
		}
		gxPopMatrix();
	}
};

struct ModelComponentType : ComponentType<ModelComponent>
{
	ModelComponentType()
	{
		typeName = "ModelComponent";
		
		in("filename", &ModelComponent::filename);
	}
};

struct ModelComponentMgr : ComponentMgr<ModelComponent>
{
	void draw() const
	{
		for (auto i = head; i != nullptr; i = i->next)
		{
			i->draw();
		}
	}
};

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
			
			//Assert(childNodeItr != scene.nodes.end());
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
			return &scene.nodes[rand() % scene.nodes.size()];
	#endif
	}
	
	void editNodeListTraverse(const int nodeId)
	{
		ImGui::PushID(nodeId);
		{
			ImGui::LabelText("Id", "%d", nodeId);
			
			//if (ImGui::BeginPopupContextWindow("NodeMenu"))
			if (ImGui::BeginPopupContextItem("NodeMenu"))
			{
				logDebug("context window for %d", nodeId);
				
				if (ImGui::MenuItem("Remove"))
				{
					nodesToRemove.insert(nodeId);
				}
				
				if (ImGui::MenuItem("Insert child node"))
				{
					SceneNode node;
					node.id = scene.allocNodeId();
					scene.nodes[node.id] = node;
					
					scene.nodes[nodeId].childNodeIds.push_back(node.id);
				}
				
				for (auto & r : s_componentTypeRegistrations)
				{
					char text[256];
					sprintf_s(text, sizeof(text), "Add %s", r.componentType->typeName.c_str());
					
					if (ImGui::MenuItem(text))
					{
						auto & node = scene.nodes[nodeId];
						auto component = r.componentMgr->createComponentForNode(node.id);
						node.components.push_back(component);
					}
				}

				ImGui::EndPopup();
			}
			
			SceneNode & node = scene.nodes[nodeId];
			
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
			
			ImGui::Indent();
			{
				if (node.childNodeIds.empty() == false)
				{
					if (ImGui::CollapsingHeader("Children"))
					{
						for (auto & childNodeId : node.childNodeIds)
						{
							auto childNodeItr = scene.nodes.find(childNodeId);
							
							//Assert(childNodeItr != scene.nodes.end());
							if (childNodeItr != scene.nodes.end())
							{
								auto childNodeId = childNodeItr->first;
								
								editNodeListTraverse(childNodeId);
							}
						}
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
				if (ImGui::Begin("Editor"))
				{
					ImGui::Checkbox("Draw ground plane", &drawGroundPlane);
					ImGui::Checkbox("Draw nodes", &drawNodes);
					ImGui::Checkbox("Draw node bounding boxes", &drawNodeBoundingBoxes);
					
					editNodeListTraverse(scene.rootNodeId);
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
				
				auto nodeItr = scene.nodes.find(nodeId);
				
				auto & node = nodeItr->second;
				
				node.freeComponents();
				
				scene.nodes.erase(nodeItr);
			}
			
			selectedNodes.clear();
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
			
			setColor(255, 0, 0, 80);
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
				
				//Assert(childNodeItr != scene.nodes.end());
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
		pushDepthTest(true, DEPTH_LESS);
		camera.pushViewMatrix();
		{
			if (drawGroundPlane)
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
			
			if (true)
			{
				s_modelComponentMgr.draw();
			}
			
			if (drawNodes)
			{
				drawNodesTraverse(scene.getRootNode());
			}
		}
		camera.popViewMatrix();
		
		popDepthTest();
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
