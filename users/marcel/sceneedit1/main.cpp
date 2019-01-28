#include "framework.h"
#include "imgui-framework.h"
#include "Parse.h"
#include "StringEx.h"
#include <map>
#include <set>

#include <typeindex>

static const int VIEW_SX = 800;
static const int VIEW_SY = 600;

struct ComponentBase;

enum ComponentPropertyType
{
	kComponentPropertyType_Int32,
	kComponentPropertyType_Float,
	kComponentPropertyType_Vec2,
	kComponentPropertyType_Vec3,
	kComponentPropertyType_Vec4,
	kComponentPropertyType_String
};

struct ComponentPropertyBase
{
	std::string name;
	ComponentPropertyType type;
	
	ComponentPropertyBase(const char * in_name, const ComponentPropertyType in_type)
		: name(in_name)
		, type(in_type)
	{
	}
};

template <typename T> ComponentPropertyType getComponentPropertyType();

template <> ComponentPropertyType getComponentPropertyType<int>()
{
	return kComponentPropertyType_Int32;
}

template <> ComponentPropertyType getComponentPropertyType<float>()
{
	return kComponentPropertyType_Float;
}

template <> ComponentPropertyType getComponentPropertyType<std::string>()
{
	return kComponentPropertyType_String;
}

template <typename T> struct ComponentProperty : ComponentPropertyBase
{
	typedef std::function<void(ComponentBase * component, const T&)> Setter;
	typedef std::function<T&(ComponentBase * component)> Getter;
	
	Getter getter;
	Setter setter;
	
	ComponentProperty(const char * name)
		: ComponentPropertyBase(name, getComponentPropertyType<T>())
	{
	}
};

typedef ComponentProperty<int> ComponentPropertyInt;
typedef ComponentProperty<float> ComponentPropertyFloat;
typedef ComponentProperty<std::string> ComponentPropertyString;

struct ComponentTypeBase
{
	typedef std::function<void(ComponentBase * component, const std::string&)> SetString;
	typedef std::function<std::string(ComponentBase * component)> GetString;
	
	/*
	struct Property
	{
		std::string name;
		ComponentPropertyType type;
		GetString getString;
		SetString setString;
	};
	*/
	
	std::string typeName;
	std::vector<ComponentPropertyBase*> properties;
	
	/*
	void genericIn(const char * name, const ComponentPropertyType type, const GetString & getString, const SetString & setString)
	{
		Property property;
		property.name = name;
		property.type = type;
		property.getString = getString;
		property.setString = setString;
		
		properties.push_back(property);
	}
	*/
};

template <typename T>
struct ComponentType : ComponentTypeBase
{
	void in(const char * name, std::string T::* member)
	{
		auto p = new ComponentPropertyString(name);
		p->getter = [=](ComponentBase * comp) -> std::string & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const std::string & s) { static_cast<T*>(comp)->*member = s; };
		
		properties.push_back(p);
	}
	
	void in(const char * name, float T::* member)
	{
		auto p = new ComponentPropertyFloat(name);
		p->getter = [=](ComponentBase * comp) -> float & { return static_cast<T*>(comp)->*member; };
		p->setter = [=](ComponentBase * comp, const float & s) { static_cast<T*>(comp)->*member = s; };
		
		properties.push_back(p);
		
	/*
		genericIn(name, kComponentPropertyType_Float,
			[=](ComponentBase * comp) -> std::string { return String::FormatC("%f", static_cast<T*>(comp)->*member); },
			[=](ComponentBase * comp, const std::string & s) { static_cast<T*>(comp)->*member = Parse::Float(s); });
	*/
	}
};

struct ComponentBase
{
	struct KeyValuePair
	{
		const char * key;
		const char * value;
	};
	
	int nodeId = -1;
	
	virtual void tick(const float dt) { }
	virtual bool init(const std::vector<KeyValuePair> & params) { return true; }
	
	virtual std::type_index typeIndex() = 0;
};

template <typename T>
struct Component : ComponentBase
{
	T * next = nullptr;
	T * prev = nullptr;
	
	virtual std::type_index typeIndex() override
	{
		return std::type_index(typeid(T));
	}
};

struct ComponentMgrBase
{
	virtual ComponentBase * createComponentForNode(const int nodeId) = 0;
	virtual void addComponentForNode(const int nodeId, ComponentBase * component) = 0;
	virtual void removeComponentForNode(const int nodeId, ComponentBase * component) = 0;
	
	virtual std::type_index typeIndex() = 0;
};

template <typename T>
struct ComponentMgr : ComponentMgrBase
{
	T * head = nullptr;
	T * tail = nullptr;
	
	virtual T * createComponentForNode(const int nodeId) override
	{
		T * component = new T();
		
		component->nodeId = nodeId;
		
		addComponentForNode(nodeId, component);
		
		return component;
	}
	
	virtual void addComponentForNode(const int nodeId, ComponentBase * in_component) override
	{
		T * component = castToComponentType(in_component);
		
		Assert(component->prev == nullptr);
		Assert(component->next == nullptr);
		
		if (head == nullptr)
		{
			head = component;
			tail = component;
		}
		else
		{
			tail->next = component;
			component->prev = tail;
			
			tail = component;
		}
	}
	
	virtual void removeComponentForNode(const int nodeId, ComponentBase * in_component) override
	{
		T * component = castToComponentType(in_component);
		
		if (component->prev != nullptr)
			component->prev->next = component->next;
		if (component->next != nullptr)
			component->next->prev = component->prev;
		
		if (component == head)
			head = component->next;
		if (component == tail)
			tail = component->prev;
		
		component->prev = nullptr;
		component->next = nullptr;
	}
	
	void tick(const float dt)
	{
		for (T * i = head; i != nullptr; i = i->next)
		{
			i->tick(dt);
		}
	}
	
	T * castToComponentType(ComponentBase * component)
	{
		return static_cast<T*>(component);
	}
	
	virtual std::type_index typeIndex() override
	{
		return std::type_index(typeid(T));
	}
};

struct ModelComponent : Component<ModelComponent>
{
	std::string filename;
	
	Vec3 aabbMin;
	Vec3 aabbMax;
	
	Vec3 position; // todo : remove. is a member of transform component
	float scale = 1.f;
	
	virtual bool init(const std::vector<KeyValuePair> & params) override
	{
		for (auto & param : params)
		{
			if (strcmp(param.key, "filename") == 0)
				filename = param.value;
			else if (strcmp(param.key, "scale") == 0)
				scale = Parse::Float(param.value);
			else if (strcmp(param.key, "x") == 0)
				position[0] = Parse::Float(param.value);
			else if (strcmp(param.key, "y") == 0)
				position[1] = Parse::Float(param.value);
			else if (strcmp(param.key, "z") == 0)
				position[2] = Parse::Float(param.value);
		}
		
		Model(filename.c_str()).calculateAABB(aabbMin, aabbMax, true);
		
		return true;
	}
	
	void draw() const
	{
		if (filename.empty())
			return;
		
		setColor(colorWhite);
		Model(filename.c_str()).drawEx(position, Vec3(0, 1, 0), 0.f, scale);
	}
};

struct ModelComponentType : ComponentType<ModelComponent>
{
	ModelComponentType()
	{
		typeName = "ModelComponent";
		
		in("filename", &ModelComponent::filename);
		in("scale", &ModelComponent::scale);
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
}

static ModelComponentMgr s_modelComponentMgr;

struct SceneNode
{
	int id = -1;
	
	Vec3 position;
	Vec3 rotation;
	
	std::vector<int> childNodeIds;
	
	std::vector<ComponentBase*> components;
	
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
	#if 1
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
					
								ImGui::InputFloat(property->name.c_str(), &value);
							}
							break;
						case kComponentPropertyType_String:
							{
								auto property = static_cast<ComponentPropertyString*>(propertyBase);
								
								auto value = property->getter(component);
					
								char buffer[1024];
								strcpy_s(buffer, sizeof(buffer), value.c_str());
								
								if (ImGui::InputText(property->name.c_str(), buffer, sizeof(buffer)))
									property->setter(component, buffer);
							}
							break;
						}
					}
				}
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
		fillCube(node.position, Vec3(.1f, .1f, .1f));
		
		const ModelComponent * modelComp = node.findComponent<ModelComponent>();
		
		if (modelComp != nullptr)
		{
			const Vec3 min = modelComp->aabbMin * modelComp->scale;
			const Vec3 max = modelComp->aabbMax * modelComp->scale;
			
			const Vec3 position = node.position + (min + max) / 2.f;
			const Vec3 size = (max - min) / 2.f;
			
			setColor(255, 0, 0, 80);
			fillCube(position, size);
		}
	}
	
	void drawNodesTraverse(const SceneNode & node) const
	{
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
		
		node.position[0] = random(-4.f, +4.f);
		node.position[1] = random(-4.f, +4.f);
		node.position[2] = random(-4.f, +4.f);
		
		auto component = s_modelComponentMgr.createComponentForNode(node.id);
		
		if (component->init({{ "filename", "model.txt" } }))
		{
			node.components.push_back(component);
			
			component->position = node.position;
			component->scale = .01f;
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
		
		s_modelComponentMgr.tick(dt);
		
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
