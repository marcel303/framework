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

#include "json.hpp"
#include "TextIO.h"

#include <algorithm>
#include <map>
#include <set>
#include <typeindex>

using namespace nlohmann;

static const int VIEW_SX = 1200;
static const int VIEW_SY = 800;

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

ComponentTypeBase * findComponentType(const std::vector<ComponentTypeRegistration> & componentTypeRegistrations, const char * typeName)
{
	for (auto & r : componentTypeRegistrations)
		if (r.componentType->typeName == typeName)
			return r.componentType;
	
	return nullptr;
}

static TransformComponentMgr s_transformComponentMgr;
static RotateTransformComponentMgr s_rotateTransformComponentMgr;
static ModelComponentMgr s_modelComponentMgr;

struct SceneNode
{
	int id = -1;
	int parentId = -1;
	std::string displayName;
	
	std::vector<int> childNodeIds;
	
	std::vector<ComponentBase*> components;
	
	Mat4x4 objectToWorld = Mat4x4(true);
	
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

void to_json(json & j, const SceneNode & node)
{
	j = json
	{
		{ "id", node.id },
		{ "displayName", node.displayName },
		{ "children", node.childNodeIds }
	};
	
	int component_index = 0;
	
	for (auto * component : node.components)
	{
		// todo : save components
		
		for (auto & r : s_componentTypeRegistrations)
		{
			if (r.componentMgr->typeIndex() == component->typeIndex())
			{
				auto & components_json = j["components"];
				
				auto & component_json = components_json[component_index++];
				
				component_json["typeName"] = r.componentType->typeName;
				
				for (auto & property : r.componentType->properties)
				{
					property->to_json(component, component_json[property->name]);
				}
				
				break;
			}
		}
	}
}

void from_json(const json & j, SceneNode & node)
{
	node.id = j.value("id", -1);
	node.displayName = j.value("displayName", "");
	node.childNodeIds = j.value("children", std::vector<int>());
	
	auto components_json_itr = j.find("components");
	
	if (components_json_itr != j.end())
	{
		auto & components_json = *components_json_itr;
		
		for (auto & component_json : components_json)
		{
			const std::string typeName = component_json.value("typeName", "");
			
			if (typeName.empty())
			{
				logWarning("empty type name");
				continue;
			}
			
			for (auto & r : s_componentTypeRegistrations)
			{
				if (r.componentType->typeName == typeName)
				{
					auto component = r.componentMgr->createComponentForNode(node.id);
					
					for (auto & property : r.componentType->properties)
					{
						if (component_json.count(property->name) != 0)
							property->from_json(component, component_json);
					}
					
					if (component->init())
						node.components.push_back(component);
					else
						r.componentMgr->removeComponentForNode(node.id, component);
				}
			}
		}
	}
}

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
	
	bool save(json & j)
	{
		j["nextNodeId"] = nextNodeId;
		j["rootNodeId"] = rootNodeId;
		
		auto & nodes_json = j["nodes"];
		
		int node_index = 0;
		
		for (auto & node_itr : nodes)
		{
			auto & node = node_itr.second;
			auto & node_json = nodes_json[node_index++];
			
			node_json = node;
		}
		
		return true;
	}
	
	bool load(const json & j)
	{
		nextNodeId = j.value("nextNodeId", -1);
		rootNodeId = j.value("rootNodeId", -1);
		
		auto nodes_vector = j.value("nodes", std::vector<SceneNode>());
		
		for (auto & node : nodes_vector)
		{
			nodes[node.id] = node;
		}
		
		for (auto & node_itr : nodes)
		{
			auto & node = node_itr.second;
			
			for (auto & childNodeId : node.childNodeIds)
			{
				auto childNode_itr = nodes.find(childNodeId);
				
				Assert(childNode_itr != nodes.end());
				if (childNode_itr != nodes.end())
				{
					auto & childNode = childNode_itr->second;
					
					childNode.parentId = node.id;
				}
			}
		}
		
		return true;
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
		}
		
		gxGetMatrixf(GX_MODELVIEW, node.objectToWorld.m_v);
		
		auto modelComp = node.findComponent<ModelComponent>();
		
		if (modelComp != nullptr)
		{
			modelComp->_objectToWorld = node.objectToWorld;
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

/**
 * Intersects a bounding box given by (min, max) with a ray specified by its origin and direction. If there is an intersection, the function returns true, and stores the distance of the point of intersection in 't'. If there is no intersection, the function returns false and leaves 't' unmodified. Note that the ray direction is expected to be the inverse of the actual ray direction, for efficiency reasons.
 * @min: Minimum of bounding box extents.
 * @max: Maximum of bounding box extents.
 * @px: Origin X of ray.
 * @py: Origin Y of ray.
 * @pz: Origin Z of ray.
 * @dxInv: Inverse of direction X of ray.
 * @dyInv: Inverse of direction Y of ray.
 * @dzInv: Inverse of direction Z of ray.
 * @t: Stores the distance to the intersection point if there is an intersection.
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
	
	Camera3d camera;
	
	Scene scene;
	
	std::set<int> selectedNodes;
	
	bool drawGroundPlane = true;
	bool drawNodes = true;
	bool drawNodeBoundingBoxes = true;
	
	FrameworkImGuiContext guiContext;
	
	std::set<int> nodesToRemove;
	
	bool cameraIsActive = false;
	
	Mat4x4 projectionMatrix;
	
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
		SceneNode * result = nullptr;
		float bestDistance = std::numeric_limits<float>::max();
		
		for (auto & nodeItr : scene.nodes)
		{
			auto & node = nodeItr.second;
			
			auto modelComponent = node.findComponent<ModelComponent>();
			
			if (modelComponent != nullptr)
			{
				auto & objectToWorld = node.objectToWorld;
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
			auto & node = nodeItr->second;
			
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
			
			node.freeComponents();
			
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
			
			removeNodeTraverse(nodeId, false);
		}

		Assert(nodesToRemove.empty());
	#if defined(DEBUG)
		for (auto & selectedNodeId : selectedNodes)
			Assert(scene.nodes.find(selectedNodeId) != scene.nodes.end());
	#endif
	}
	
	void editNode(SceneNode & node, const bool editChildren)
	{
		char name[256];
		sprintf_s(name, sizeof(name), "%s", node.displayName.c_str());
		if (ImGui::InputText("Name", name, sizeof(name)))
			node.displayName = name;
		
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
								
								ImGui::SliderAngle(property->name.c_str(), &value.angle);
								ImGui::PushID(&value.axis);
								ImGui::SliderFloat3(property->name.c_str(), &value.axis[0], -1.f, +1.f);
								ImGui::PopID();
							}
							break;
						}
					}
				}
			}
			ImGui::PopID();
		}
	
		if (editChildren)
		{
			ImGui::Indent();
			{
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
							editNodeListTraverse(childNodeId, editChildren);
						}
				
						ImGui::PopID();
					}
				}
			}
			ImGui::Unindent();
		}
	}
	
	void editNodeListTraverse(const int nodeId, const bool editChildren)
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
							if (component->init())
								node.components.push_back(component);
							else
								r.componentMgr->removeComponentForNode(node.id, component);
						}
					}
				}

				ImGui::EndPopup();
			}
			
			editNode(node, editChildren);
		}
		ImGui::PopID();
	}
	
	void addNodeFromTemplate(Vec3Arg position, const AngleAxis & angleAxis, const int parentId)
	{
		SceneNode node;
		
		node.id = scene.allocNodeId();
		node.parentId = parentId;
		node.displayName = String::FormatC("Node %d", node.id);
		
	// todo : create the node from an actual template
		auto modelComponentType = findComponentType(s_componentTypeRegistrations, "ModelComponent");
		
		auto modelComp = s_modelComponentMgr.createComponentForNode(node.id);
		
		if (modelComponentType->initComponent(modelComp, { { "filename", "model.txt" }, { "scale", "0.01" } }) &&
			modelComp->init())
			node.components.push_back(modelComp);
		else
			s_modelComponentMgr.removeComponentForNode(node.id, modelComp);
		
		auto transformComp = s_transformComponentMgr.createComponentForNode(node.id);
		
		if (transformComp->init())
		{
			transformComp->position = position;
			
			node.components.push_back(transformComp);
		}
		else
			s_transformComponentMgr.removeComponentForNode(node.id, transformComp);
		
		scene.nodes[node.id] = node;
		
		//
		
		auto parentNode_itr = scene.nodes.find(parentId);
		Assert(parentNode_itr != scene.nodes.end());
		if (parentNode_itr != scene.nodes.end())
		{
			auto & parentNode = parentNode_itr->second;
			
			parentNode.childNodeIds.push_back(node.id);
		}
	}
	
	void tickEditor(const float dt, bool & inputIsCaptured)
	{
	// todo : this is a bit of a hack. avoid projectPerspective3d altogether ?
		projectPerspective3d(60.f, .1f, 100.f);
		gxGetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
		projectScreen2d();
		
		if (cameraIsActive == false)
		{
			guiContext.processBegin(dt, 800, 600, inputIsCaptured);
			{
				if (ImGui::Begin("Editor", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
				{
					ImGui::Checkbox("Draw ground plane", &drawGroundPlane);
					ImGui::Checkbox("Draw nodes", &drawNodes);
					ImGui::Checkbox("Draw node bounding boxes", &drawNodeBoundingBoxes);
					
					if (selectedNodes.empty())
					{
						ImGui::Text("Root node");
						editNodeListTraverse(scene.rootNodeId, true);
					}
					else
					{
						for (auto & selectedNodeId : selectedNodes)
						{
							editNodeListTraverse(selectedNodeId, false);
						}
					}
					
					removeNodesToRemove();
				}
				ImGui::End();
			}
			guiContext.processEnd();
		}
		
		if (inputIsCaptured == false && mouse.wentDown(BUTTON_LEFT))
		{
			//inputIsCaptured = true;
			
			// transform mouse coordinates into a world space direction vector
			
			const Vec2 mousePosition_screen(
				mouse.x,
				mouse.y);
			const Vec2 mousePosition_clip(
				mousePosition_screen[0] / float(VIEW_SX) * 2.f - 1.f,
				mousePosition_screen[1] / float(VIEW_SY) * 2.f - 1.f);
			const Vec2 mousePosition_view = projectionMatrix.CalcInv().Mul(mousePosition_clip);
			const Vec3 mouseDirection_world = camera.getWorldMatrix().Mul3(
				Vec3(
					+mousePosition_view[0],
					-mousePosition_view[1],
					1.f));
			
			if (keyboard.isDown(SDLK_LSHIFT))
			{
				// todo : make action dependent on editing state. in this case, node placement
			
				// find intersection point with the ground plane
			
				if (mouseDirection_world[1] != 0.f)
				{
					const float t = -camera.position[1] / mouseDirection_world[1];
					
					if (t >= 0.f)
					{
						const Vec3 groundPosition = camera.position + mouseDirection_world * t;
						
						if (keyboard.isDown(SDLK_c))
						{
							for (auto & parentNodeId : selectedNodes)
							{
								auto parentNode_itr = scene.nodes.find(parentNodeId);
								Assert(parentNode_itr != scene.nodes.end());
								if (parentNode_itr != scene.nodes.end())
								{
									auto & parentNode = parentNode_itr->second;
									
									const Vec3 groundPosition_parent = parentNode.objectToWorld.CalcInv().Mul4(groundPosition);
									
									addNodeFromTemplate(groundPosition_parent, AngleAxis(), parentNodeId);
								}
							}
						}
						else
						{
							addNodeFromTemplate(groundPosition, AngleAxis(), scene.rootNodeId);
						}
					}
				}
			}
			else
			{
				const SceneNode * node = raycast(camera.position, mouseDirection_world);
				
				selectedNodes.clear();
				
				if (node != nullptr)
					selectedNodes.insert(node->id);
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
			
			setColor(isSelected ? 255 : 60, 0, 0, 40);
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
		
		auto modelComponentType = findComponentType(s_componentTypeRegistrations, "ModelComponent");
		
		if (modelComponentType->initComponent(modelComp, { { "filename", "model.txt" }, { "scale", "0.01" } }) &&
			modelComp->init())
			node.components.push_back(modelComp);
		else
			s_modelComponentMgr.removeComponentForNode(node.id, modelComp);
		
		auto transformComp = s_transformComponentMgr.createComponentForNode(node.id);
		
		if (transformComp->init())
		{
			transformComp->position[0] = random(-4.f, +4.f);
			transformComp->position[1] = random(-4.f, +4.f);
			transformComp->position[2] = random(-4.f, +4.f);
			
			node.components.push_back(transformComp);
		}
		else
			s_transformComponentMgr.removeComponentForNode(node.id, transformComp);
		
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
	framework.allowHighDpi = false;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	registerComponentType(new TransformComponentType(), &s_transformComponentMgr);
	registerComponentType(new RotateTransformComponentType(), &s_rotateTransformComponentMgr);
	registerComponentType(new ModelComponentType(), &s_modelComponentMgr);
	
	SceneEditor editor;
	editor.init();
	
	//createRandomScene(editor.scene);
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		const float dt = framework.timeStep;
		
		bool inputIsCaptured = false;
		
		editor.tickEditor(dt, inputIsCaptured);
		
		// save load test. todo : remove test code
		
		if (keyboard.wentDown(SDLK_s))
		{
			json j;
			
			if (editor.scene.save(j))
			{
				auto text = j.dump(4);
				
				printf("%s", text.c_str());
				
				std::vector<std::string> lines;
				TextIO::LineEndings lineEndings;
				if (TextIO::loadText(text.c_str(), lines, lineEndings))
				{
					TextIO::save("testScene.json", lines, TextIO::kLineEndings_Unix);
				}
				
				//
				
				Scene tempScene;
				
				if (tempScene.load(j))
				{
					for (auto & node_itr : editor.scene.nodes)
						editor.nodesToRemove.insert(node_itr.second.id);
					editor.removeNodesToRemove();
					
					editor.scene = tempScene;
				}
			}
		}
		
		if (keyboard.wentDown(SDLK_l))
		{
			bool result = true;
			
			char * text;
			size_t textSize;
			
			if (result == true)
			{
				result &= TextIO::loadFileContents("testScene.json", text, textSize);
			}
			
			json j;
			
			if (result == true)
			{
				try
				{
					j = json::parse(text);
				}
				catch (std::exception & e)
				{
					logError("failed to parse JSON: %s", e.what());
					result &= false;
				}
				
				delete [] text;
				text = nullptr;
			}
			
			Scene tempScene;
			
			if (result == true)
			{
				result &= tempScene.load(j);
			}
			
			if (result == true)
			{
				for (auto & node_itr : editor.scene.nodes)
					editor.nodesToRemove.insert(node_itr.second.id);
				editor.removeNodesToRemove();
				
				editor.scene = tempScene;
			}
		}
		
		//
		
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
