#include "cameraComponent.h"
#include "modelComponent.h"
#include "parameterComponent.h"
#include "parameterUi.h"
#include "transformComponent.h"

#include "component.h"
#include "componentPropertyUi.h"
#include "componentType.h"
#include "framework.h"
#include "imgui-framework.h"
#include "scene.h"
#include "scene_fromText.h"
#include "StringEx.h"
#include "TextIO.h"

#include "helpers.h"
#include "template.h"

#include <algorithm>
#include <limits>
#include <set>
#include <typeindex>

extern void test_templates();
extern bool test_scenefiles();
extern bool test_templateEditor();
extern void test_reflection_1();

static const int VIEW_SX = 1200;
static const int VIEW_SY = 800;

// todo : move these instances to helpers.cpp
TransformComponentMgr s_transformComponentMgr;
ModelComponentMgr s_modelComponentMgr;
extern ParameterComponentMgr s_parameterComponentMgr;

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
			auto & node = *nodeItr.second;
			
			auto modelComponent = node.components.find<ModelComponent>();
			
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
	
	void removeNodesToRemove()
	{
		// todo : remove nodesToRemove and directly call removeNodeTraverse ?
		
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
	
	void editNode(SceneNode & node, const bool editChildren)
	{
		// todo : make a separate function to edit a data structure (recursively)
	#if 1
		for (auto * component = node.components.head; component != nullptr; component = component->next_in_set)
		{
			ImGui::PushID(component);
			{
				auto * componentType = findComponentType(component->typeIndex());
				
				Assert(componentType != nullptr);
				if (componentType != nullptr)
				{
					ImGui::LabelText("Component", "%s", componentType->typeName);
					
					bool isSet = true;
					void * changedMemberObject = nullptr;
					
					if (doReflection_StructuredType(g_typeDB, *componentType, component, isSet, &changedMemberObject))
					{
						// signal component one of its properties has changed
						component->propertyChanged(changedMemberObject);
					}
				}
			}
			ImGui::PopID();
		}
	#endif
	
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
						
						auto & childNode = *childNodeItr->second;
						
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
		
		SceneNode & node = *nodeItr->second;
		
		ImGui::PushID(nodeId);
		{
			char name[256];
			sprintf_s(name, sizeof(name), "%s", node.displayName.c_str());
			if (ImGui::InputText("Name", name, sizeof(name)))
				node.displayName = name;
			
			if (ImGui::BeginPopupContextItem("NodeMenu"))
			{
				//logDebug("context window for %d", nodeId);
				
				if (ImGui::MenuItem("Remove"))
				{
					nodesToRemove.insert(nodeId);
				}
				
				if (ImGui::MenuItem("Insert node"))
				{
					SceneNode & childNode = *new SceneNode();
					childNode.id = scene.allocNodeId();
					childNode.parentId = nodeId;
					childNode.displayName = String::FormatC("Node %d", childNode.id);
					scene.nodes[childNode.id] = &childNode;
					
					scene.nodes[nodeId]->childNodeIds.push_back(childNode.id);
				}
				
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
							auto * component = componentType->componentMgr->createComponent(nullptr);
							
							if (component->init())
								node.components.add(component);
							else
								componentType->componentMgr->destroyComponent(component);
						}
					}
				}

				ImGui::EndPopup();
			}
			
			editNode(node, editChildren);
		}
		ImGui::PopID();
	}
	
	void addNodeFromTemplate_v1(Vec3Arg position, const AngleAxis & angleAxis, const int parentId)
	{
		SceneNode & node = *new SceneNode();
		
		node.id = scene.allocNodeId();
		node.parentId = parentId;
		node.displayName = String::FormatC("Node %d", node.id);
		
		auto modelComp = s_modelComponentMgr.createComponent(nullptr);
		
		modelComp->filename = "model.txt";
		modelComp->scale = .01f;
		
		if (modelComp->init())
			node.components.add(modelComp);
		else
			s_modelComponentMgr.destroyComponent(modelComp);
		
		auto transformComp = s_transformComponentMgr.createComponent(nullptr);
		
		if (transformComp->init())
		{
			transformComp->position = position;
			
			node.components.add(transformComp);
		}
		else
			s_transformComponentMgr.destroyComponent(transformComp);
		
		scene.nodes[node.id] = &node;
		
		//
		
		auto parentNode_itr = scene.nodes.find(parentId);
		Assert(parentNode_itr != scene.nodes.end());
		if (parentNode_itr != scene.nodes.end())
		{
			auto & parentNode = *parentNode_itr->second;
			
			parentNode.childNodeIds.push_back(node.id);
		}
	}
	
	void addNodeFromTemplate_v2(Vec3Arg position, const AngleAxis & angleAxis, const int parentId)
	{
		Template t;
		
		if (!loadTemplateWithOverlaysFromFile("textfiles/base-entity-v1-overlay.txt", t, false))
			return;
		
		//
		
		SceneNode * node = new SceneNode();
		
		node->id = scene.allocNodeId();
		node->parentId = parentId;
		node->displayName = String::FormatC("Node %d", node->id);
		
		bool init_ok = true;
		
		init_ok &= instantiateComponentsFromTemplate(g_typeDB, t, node->components);
		
		for (auto * component = node->components.head; component != nullptr && init_ok; component = component->next_in_set)
		{
			init_ok &= component->init();
		}
		
		if (init_ok == false)
		{
			logError("failed to initialize node components");
			
			node->freeComponents();
			
			delete node;
			node = nullptr;
			
			return;
		}
		
		{
			auto transformComp = node->components.find<TransformComponent>();
			
			transformComp->position = position;
			transformComp->angleAxis = angleAxis;
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
	}
	
	void tickEditor(const float dt, bool & inputIsCaptured)
	{
	// todo : this is a bit of a hack. avoid projectPerspective3d altogether ?
		projectPerspective3d(60.f, .1f, 100.f);
		gxGetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
		projectScreen2d();
		
		if (cameraIsActive == false)
		{
			guiContext.processBegin(dt, VIEW_SX, VIEW_SY, inputIsCaptured);
			{
				ImGui::SetNextWindowPos(ImVec2(4, 4));
				ImGui::SetNextWindowSize(ImVec2(370, VIEW_SY * 2/3));
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
				
				//
				
				if (ImGui::Begin("Parameter UI"))
				{
					for (auto * component = s_parameterComponentMgr.head; component != nullptr; component = component->next)
					{
						ImGui::Text("%s", component->prefix.c_str());
						
						doParameterUi(*component);
					}
				}
				ImGui::End();
			}
			guiContext.processEnd();
		}
		
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
		
		// determine which node is underneath the mouse cursor
		
		const SceneNode * hoverNode = raycast(camera.position, mouseDirection_world);
		
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
									auto & parentNode = *parentNode_itr->second;
									
									const Vec3 groundPosition_parent = parentNode.objectToWorld.CalcInv().Mul4(groundPosition);
									
									addNodeFromTemplate_v2(groundPosition_parent, AngleAxis(), parentNodeId);
								}
							}
						}
						else
						{
							addNodeFromTemplate_v2(groundPosition, AngleAxis(), scene.rootNodeId);
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
		
	#if 0 // todo : remove ?
		const CameraComponent * cameraComp = node.components.find<CameraComponent>();
		
		if (cameraComp != nullptr)
		{
		// todo : draw camera frustum
		
			const Vec3 origin = cameraComp->viewMatrix.Mul4(Vec3(0, 0, 0));
			
			gxBegin(GX_LINES);
			{
				for (int dx = -1; dx <= +1; dx += 2)
				{
					for (int dy = -1; dy <= +1; dy += 2)
					{
						const int dz = 1;
						
						const Vec3 vertex(dx, dy, dz);
						
						setColor(colorWhite);
						gxVertex3f(origin[0], origin[1], origin[2]);
						gxVertex3f(vertex[0], vertex[1], vertex[2]);
					}
				}
			}
			gxEnd();
		}
	#endif
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
		projectPerspective3d(60.f, .1f, 100.f);
		camera.pushViewMatrix();
		{
			if (true)
			{
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_OPAQUE);
				s_modelComponentMgr.draw(scene);
				popBlend();
				popDepthTest();
			}
			
			if (drawGroundPlane)
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

#include "cameraResource.h"

static void testResources()
{
	g_resourceDatabase.add("controller 1", new Resource<CameraControllerTest>());
	g_resourceDatabase.add("controller 2", new Resource<CameraControllerTest>());
}

//

#if 00

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
			
			if (editor.scene.saveToFile("testScene.json"))
			{
				Scene tempScene;
				
				if (tempScene.loadFromFile("testScene.json"))
				{
					for (auto & node_itr : editor.scene.nodes)
						editor.nodesToRemove.insert(node_itr.second->id);
					editor.removeNodesToRemove();
					
					editor.scene = tempScene;
				}
			}
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
		
		//
		
		s_transformComponentMgr.calculateTransforms(editor.scene);
		
		for (auto * type : g_componentTypes)
		{
			type->componentMgr->tick(dt);
		}
		
		s_transformComponentMgr.calculateTransforms(editor.scene);
		
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
