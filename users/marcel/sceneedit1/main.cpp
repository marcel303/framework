#include "component.h"
#include "componentPropertyUi.h"
#include "componentType.h"
#include "framework.h"
#include "framework-camera.h"
#include "gx_render.h"
#include "imgui-framework.h"
#include "parameterComponentUi.h"
#include "parameterUi.h"
#include "Path.h"
#include "Quat.h"
#include "raycast.h"
#include "scene.h"
#include "scene_fromText.h"
#include "scene_clipboardHelpers.h"
#include "StringEx.h"
#include "TextIO.h"
#include "transformGizmos.h"

// component types
#include "components/cameraComponent.h"
#include "components/lightComponent.h"
#include "components/modelComponent.h"
#include "components/parameterComponent.h"
#include "components/sceneNodeComponent.h"
#include "components/transformComponent.h"

#include "helpers.h"
#include "lineReader.h"
#include "lineWriter.h"
#include "template.h"
#include "templateIo.h"

#include <algorithm>
#include <limits>
#include <set>
#include <typeindex>

#include <SDL2/SDL.h>

#define ENABLE_TRANSFORM_GIZMOS 1

#define ENABLE_QUAT_FIXUP 1

#if defined(DEBUG)
	#define ENABLE_LOAD_AFTER_SAVE_TEST 1
	#define ENABLE_PERFORMANCE_TEST     1
	#define ENABLE_TEMPLATE_TEST        1
	#define ENABLE_SAVE_LOAD_TIMING     1
#else
	#define ENABLE_LOAD_AFTER_SAVE_TEST 0 // do not alter
	#define ENABLE_PERFORMANCE_TEST     0 // do not alter
	#define ENABLE_TEMPLATE_TEST        0 // do not alter
	#define ENABLE_SAVE_LOAD_TIMING     0 // do not alter
#endif

/*

todo :

- add lookat/focus option to scene structure view
	- as a context menu
- add 'Paste tree' to scene structure context menu
- add support for copying node tree 'Copy tree'

- add template file list / browser
- add support for template editing
- add option to add nodes from template

done :

+ add libreflection-jsonio
+ remove json references from scene edit
+ add general monitor camera. or extend framework's Camera3d to support perspective, orbit and ortho modes
+ avoid UI from jumping around
	+ add independent scrolling area for scene structure
	+ add independent scrolling area for selected node
+ use scene structure tree to select nodes. remove inline editing
+ separate node traversal/scene structure view from node editor
+ add 'path' editor type hint. show a browser button and prompt a file dialog when pressed

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
extern LightComponentMgr s_lightComponentMgr;
extern ModelComponentMgr s_modelComponentMgr;
extern ParameterComponentMgr s_parameterComponentMgr;
extern SceneNodeComponentMgr s_sceneNodeComponentMgr;

//

#include <functional>

typedef std::function<void()> RenderSceneCallback;

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
	ParameterBool * anaglyphic = nullptr;
	ParameterFloat * eyeSeparation = nullptr;
	
	RenderSceneCallback drawOpaque = nullptr;
	RenderSceneCallback drawTranslucent = nullptr;
	
	mutable DepthTarget depthMap;
	mutable ColorTarget normalMap;
	mutable ColorTarget colorMap;
	mutable ColorTarget lightMap;
	mutable ColorTarget compositeMap;
	
	mutable ColorTarget eyeLeft;
	mutable ColorTarget eyeRight;
	mutable DepthTarget eyeDepth;
	
	struct
	{
		Mat4x4 projectionMatrix;
		Mat4x4 viewMatrix;
	} mutable drawState;
	
	bool init()
	{
		bool result = true;

		parameterMgr.init("renderer");
		
		mode = parameterMgr.addEnum("mode", kMode_Lit,
			{
				{ "Wireframe", kMode_Wireframe },
				{ "Colors", kMode_Colors },
				{ "Normals", kMode_Normals },
				{ "Lit", kMode_Lit },
				{ "Lit + Shadows", kMode_LitWithShadows }
			});
		
		anaglyphic = parameterMgr.addBool("anaglyphic", false);
		
		/*
		From Wikipedia, https://en.wikipedia.org/wiki/Pupillary_distance#Measuring_pupillary_distance
		
		Interpupillary distance (IPD)
		(distance in mm)
		
		Gender  Sample size Mean  Standard deviation  Minimum  Maximum
		Female  1986        61.7  3.6                 51.0     74.5
		Male    4082        64.0  3.4                 53.0     77.0
		*/
		
		eyeSeparation = parameterMgr.addFloat("eyeSepration", .062f);
		eyeSeparation->setLimits(0.f, .1f);
		
		result &= depthMap.init(VIEW_SX, VIEW_SY, DEPTH_FLOAT32, true, 1.f);
		result &= normalMap.init(VIEW_SX, VIEW_SY, SURFACE_RGBA16F, colorBlackTranslucent);
		result &= colorMap.init(VIEW_SX, VIEW_SY, SURFACE_RGBA8, colorBlackTranslucent);
		result &= lightMap.init(VIEW_SX, VIEW_SY, SURFACE_RGBA16F, colorBlackTranslucent);
		result &= compositeMap.init(VIEW_SX, VIEW_SY, SURFACE_RGBA8, colorBlackTranslucent);
		
		result &= eyeLeft.init(VIEW_SX, VIEW_SY, SURFACE_RGBA8, colorBlackTranslucent);
		result &= eyeRight.init(VIEW_SX, VIEW_SY, SURFACE_RGBA8, colorBlackTranslucent);
		result &= eyeDepth.init(VIEW_SX, VIEW_SY, DEPTH_FLOAT32, true, 1.f);
		
		return result;
	}
	
	void drawColorPass() const
	{
		pushShaderOutputs("c");
		{
			if (drawOpaque != nullptr)
				drawOpaque();
		}
		popShaderOutputs();
	}
	
	void drawNormalPass() const
	{
		pushShaderOutputs("n");
		{
			if (drawOpaque != nullptr)
				drawOpaque();
		}
		popShaderOutputs();
	}
	
	void drawTranslucentPass() const
	{
		pushDepthTest(true, DEPTH_LESS, false);
		pushBlend(BLEND_ALPHA);
		{
			if (drawTranslucent != nullptr)
				drawTranslucent();
		}
		popBlend();
		popDepthTest();
	}
	
	void pushMatrices(const bool drawToSurface) const
	{
		applyTransform();
		gxMatrixMode(GX_PROJECTION);
		gxPushMatrix();
		gxLoadMatrixf(drawState.projectionMatrix.m_v);
		
		gxMatrixMode(GX_MODELVIEW);
		gxPushMatrix();
		gxLoadMatrixf(drawState.viewMatrix.m_v);
	}
	
	void popMatrices() const
	{
		gxMatrixMode(GX_PROJECTION);
		gxPopMatrix();
		gxMatrixMode(GX_MODELVIEW);
		gxPopMatrix();
	}
	
	void draw(const Mat4x4 & projectionMatrix, const Mat4x4 & viewMatrix) const
	{
		if (anaglyphic->get())
		{
			drawFromEye(projectionMatrix, viewMatrix, Vec3(-eyeSeparation->get()/2.f, 0.f, 0.f), &eyeLeft, &eyeDepth);
			drawFromEye(projectionMatrix, viewMatrix, Vec3(+eyeSeparation->get()/2.f, 0.f, 0.f), &eyeRight, &eyeDepth);
			
			// composite eye buffers using anaglyphic shader
			
			projectScreen2d();
			
			pushBlend(BLEND_OPAQUE);
			{
				Shader shader("shaders/anaglyphic-compose");
				setShader(shader);
				{
					shader.setImmediate("mode", 0);
					shader.setTexture("colormapL", 0, eyeLeft.getTextureId(), false);
					shader.setTexture("colormapR", 1, eyeRight.getTextureId(), false);
					
					drawRect(0, 0, eyeLeft.getWidth(), eyeLeft.getHeight());
				}
				clearShader();
			}
			popBlend();
		}
		else
		{
			drawFromEye(projectionMatrix, viewMatrix, Vec3(), nullptr, nullptr);
		}
	}
	
	void drawFromEye(
		const Mat4x4 & projectionMatrix,
		const Mat4x4 & viewMatrix,
		Vec3Arg eyeOffset,
		ColorTarget * colorTarget,
		DepthTarget * depthTarget) const
	{
		drawState.projectionMatrix = projectionMatrix.Translate(eyeOffset);
		drawState.viewMatrix = viewMatrix;
		
		if (mode->get() == kMode_Wireframe)
		{
			if (colorTarget != nullptr)
				pushRenderPass(colorTarget, true, depthTarget, true, "Wireframe");
			
			pushMatrices(false);
			pushWireframe(true);
			{
				drawColorPass();
				drawTranslucentPass();
			}
			popWireframe();
			popMatrices();
			
			if (colorTarget != nullptr)
				popRenderPass();
		}
		else if (mode->get() == kMode_Colors)
		{
			if (colorTarget != nullptr)
				pushRenderPass(colorTarget, true, depthTarget, true, "Colors");
			
			pushMatrices(false);
			{
				drawColorPass();
				drawTranslucentPass();
			}
			popMatrices();
			
			if (colorTarget != nullptr)
				popRenderPass();
		}
		else if (mode->get() == kMode_Normals)
		{
			if (colorTarget != nullptr)
				pushRenderPass(colorTarget, true, depthTarget, true, "Normals");
			
			pushMatrices(false);
			{
				drawNormalPass();
				drawTranslucentPass();
			}
			popMatrices();
			
			if (colorTarget != nullptr)
				popRenderPass();
		}
		else if (mode->get() == kMode_Lit || mode->get() == kMode_LitWithShadows)
		{
			// todo : if shadows are enabled, render the shadow maps here
			
			// draw to the normal and depth maps
			
			ColorTarget * targets[2] = { &normalMap, &colorMap };
			
			pushRenderPass(targets, 2, true, &depthMap, true, "Depth Normal Color");
			pushShaderOutputs("nc");
			{
				pushMatrices(true);
				{
					if (drawOpaque != nullptr)
						drawOpaque();
				}
				popMatrices();
			}
			popShaderOutputs();
			popRenderPass();
		
			// accumulate lights for the opaque part of the scene into the light map
			
			pushRenderPass(&lightMap, true, nullptr, false, "Light");
			pushBlend(BLEND_ADD_OPAQUE);
			{
				projectScreen2d();
				
				// draw lights using depth and normal maps as inputs
				
				for (auto * light = s_lightComponentMgr.head; light != nullptr; light = light->next)
				{
					if (light->type == LightComponent::kLightType_Point)
					{
						// draw point light
						
						auto * sceneNode = light->componentSet->find<SceneNodeComponent>();
						
						Vec3 lightPosition_world;
						
						if (sceneNode != nullptr)
						{
							lightPosition_world = sceneNode->objectToWorld.GetTranslation();
						}
						
						const Vec3 lightPosition_view = viewMatrix.Mul4(lightPosition_world);
						const Vec3 lightColor = light->color * light->intensity;
						
						Shader shader("shaders/point-light");
						setShader(shader);
						shader.setTexture("depthTexture", 0, depthMap.getTextureId(), false, true);
						shader.setTexture("normalTexture", 1, normalMap.getTextureId(), false, true);
						shader.setImmediateMatrix4x4("projectionToView", projectionMatrix.CalcInv().m_v);
						shader.setImmediate("lightPosition_view",
							lightPosition_view[0],
							lightPosition_view[1],
							lightPosition_view[2]);
						shader.setImmediate("lightColor",
							lightColor[0],
							lightColor[1],
							lightColor[2]);
						shader.setImmediate("lightAttenuationParams",
							light->innerRadius,
							light->outerRadius);
						drawRect(0, 0, lightMap.getWidth(), lightMap.getHeight());
						clearShader();
					}
					else if (light->type == LightComponent::kLightType_Directional)
					{
						// draw full screen directional light
						
						Vec3 lightDir_world(0, 0, 1);
						
						auto * sceneNode = light->componentSet->find<SceneNodeComponent>();
						
						if (sceneNode != nullptr)
						{
							lightDir_world = sceneNode->objectToWorld.Mul3(lightDir_world);
						}
						
						const Vec3 lightDir_view = viewMatrix.Mul3(lightDir_world);
						const Vec3 lightColor1 = light->color * light->intensity; // light color when the light is coming from 'above'
						const Vec3 lightColor2 = light->bottomColor * light->intensity; // light color when the light is coming from 'below'
						
						Shader shader("shaders/directional-light");
						setShader(shader);
						shader.setTexture("depthTexture", 0, depthMap.getTextureId(), false, true);
						shader.setTexture("normalTexture", 1, normalMap.getTextureId(), false, true);
						shader.setImmediateMatrix4x4("projectionToView", projectionMatrix.CalcInv().m_v);
						shader.setImmediate("lightDir_view", lightDir_view[0], lightDir_view[1], lightDir_view[2]);
						shader.setImmediate("lightColor1", lightColor1[0], lightColor1[1], lightColor1[2]);
						shader.setImmediate("lightColor2", lightColor2[0], lightColor2[1], lightColor2[2]);
						drawRect(0, 0, lightMap.getWidth(), lightMap.getHeight());
						clearShader();
					}
				}
			}
			popBlend();
			popRenderPass();
			
			// perform light application. the light application pass combines
			// the color and light maps to produce a lit result
			
			pushRenderPass(&compositeMap, true, nullptr, false, "Composite");
			{
				projectScreen2d();
				
				Shader shader("shaders/light-application");
				setShader(shader);
				shader.setTexture("colorTexture", 0, colorMap.getTextureId(), false, true);
				shader.setTexture("lightTexture", 1, lightMap.getTextureId(), false, true);
				drawRect(0, 0, compositeMap.getWidth(), compositeMap.getHeight());
				clearShader();
			}
			popRenderPass();
			
			// draw translucent pass on top of the composited result

			pushRenderPass(&compositeMap, false, &depthMap, false, "Translucent");
			{
				pushMatrices(true);
				{
					drawTranslucentPass();
				}
				popMatrices();
			}
			popRenderPass();
			
			projectScreen2d();
			
			if (colorTarget != nullptr)
				pushRenderPass(colorTarget, true, depthTarget, true, mode->get() == kMode_Lit ? "Lit" : "LitWithShadows");
			
			pushBlend(BLEND_OPAQUE);
			gxSetTexture(compositeMap.getTextureId());
			setColor(colorWhite);
		#if ENABLE_OPENGL
			drawRect(0, compositeMap.getHeight(), compositeMap.getWidth(), 0);
		#else
			drawRect(0, 0, compositeMap.getWidth(), compositeMap.getHeight());
		#endif
			gxSetTexture(0);
			popBlend();
			
			if (colorTarget != nullptr)
				popRenderPass();
		}
	}
};

//

struct SceneEditor;

struct SceneEditor
{
	static const int kMaxNodeDisplayNameFilter = 100;
	static const int kMaxComponentTypeNameFilter = 100;
	static const int kMaxParameterFilter = 100;
	
	Scene scene; // todo : this should live outside the editor, but referenced
	
	Camera camera; // todo : this should live outside the editor, but referenced
	bool cameraIsActive = false;
	
	std::set<int> selectedNodes;
	int hoverNodeId = -1;
	
	int nodeToGiveFocus = -1;
	bool enablePadGizmo = false;
	
#if ENABLE_TRANSFORM_GIZMOS
	TransformGizmo transformGizmo;
#endif

	Renderer renderer; // todo : this should live outside the editor
	
	FrameworkImGuiContext guiContext;
	
	struct
	{
		bool drawGrid = true;
		bool drawGroundPlane = false;
		bool drawNodes = true;
		bool drawNodeBoundingBoxes = true;
	} visibility;
	
	struct
	{
		bool tickScene = false;
		float tickMultiplier = 1.f;
		
		bool drawScene = true;
	} preview;
	
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
	
	struct
	{
		char nodeDisplayNameFilter[kMaxNodeDisplayNameFilter] = { };
		char componentTypeNameFilter[kMaxComponentTypeNameFilter] = { };
		std::set<int> visibleNodes; // set of visible nodes, when the node structure is filtered, by for instance node name or component type
		std::set<int> nodesToOpen; // nodes inside this set will be forcibly opened inside the node structure editor and scrolled into view
	} nodeUi;
	
	struct
	{
		char component_filter[kMaxParameterFilter] = { };
		char parameter_filter[kMaxParameterFilter] = { };
		
		bool showAnonymousComponents = false;
	} parameterUi;
	
	struct
	{
		std::vector<std::string> versions;
		int currentVersionIndex = -1;
		
		std::string currentVersion;
	} undo;
	
	//
	
	void init()
	{
		scene.createRootNode();
		
		camera.mode = Camera::kMode_Orbit;
		camera.orbit.distance = -8.f;
		camera.ortho.side = Camera::kOrthoSide_Top;
		camera.ortho.scale = 16.f;
		camera.firstPerson.position = Vec3(0, 1, -2);
		camera.firstPerson.height = 0.f;
		
		renderer.init();
		renderer.drawOpaque = [&]() { drawOpaque(); };
		renderer.drawTranslucent = [&]() { drawTranslucent(); };
		
		guiContext.init();
	}
	
	void shut()
	{
		scene.freeAllNodesAndComponents();
		
		// todo : shutdown renderer
		
		guiContext.shut();
	}
	
	SceneNode * raycast(Vec3Arg rayOrigin, Vec3Arg rayDirection, const std::set<int> & nodesToIgnore) const
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
			if (sceneNodeComp != nullptr && modelComponent != nullptr)
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
	void deferredBegin()
	{
		Assert(deferred.isFlushed());
	}
	
	void deferredEnd(const bool selectAddedNodes)
	{
		if (deferred.isFlushed() == false)
		{
			std::set<int> nodesToSelect;
			
			if (selectAddedNodes)
			{
				for (auto * node : deferred.nodesToAdd)
					nodesToSelect.insert(node->id);
			}
			
			//
			
			undoCaptureBegin();
			{
				addNodesToAdd();
				
				removeNodesToRemove();
			}
			undoCaptureEnd();
			
			//
			
			if (selectAddedNodes)
			{
				selectNodes(nodesToSelect, false);
			}
		}
	}
	
	void removeNodeSubTree(const int nodeId)
	{
		auto & node = scene.getNode(nodeId);
		
		// recursively remove child nodes
		
		while (!node.childNodeIds.empty())
		{
			const int childNodeId = node.childNodeIds.front();
			removeNodeSubTree(childNodeId);
		}
		Assert(node.childNodeIds.empty());
		
		// remove our reference from our parent
		
		if (node.parentId != -1)
		{
			auto & parentNode = scene.getNode(node.parentId);
			auto childNodeId_itr = std::find(parentNode.childNodeIds.begin(), parentNode.childNodeIds.end(), node.id);
			Assert(childNodeId_itr != parentNode.childNodeIds.end());
			parentNode.childNodeIds.erase(childNodeId_itr);
		}
		
		// free the node
		
		scene.freeNode(nodeId);
		
		// update node references
		
		if (nodeId == scene.rootNodeId)
			scene.rootNodeId = -1;
		
		selectedNodes.erase(nodeId);
		
		deferred.nodesToRemove.erase(nodeId);
	}
	
	void selectNodes(const std::set<int> & nodeIds, const bool append)
	{
		if (append == false)
		{
			selectedNodes.clear();
		}
		
		for (auto & nodeId : nodeIds)
		{
			selectedNodes.insert(nodeId);
			markNodeOpenUntilRoot(nodeId);
			nodeToGiveFocus = nodeId;
			enablePadGizmo = true;
		}
	}
	
	void selectNode(const int nodeId, const bool append)
	{
		selectNodes({ nodeId }, append);
	}
	
	void addNodesToAdd()
	{
		for (SceneNode * node : deferred.nodesToAdd)
		{
			Assert(scene.nodes.find(node->id) == scene.nodes.end());
			scene.nodes[node->id] = node;
			
			// insert the node into the scene
			auto & parentNode = scene.getNode(node->parentId);
			parentNode.childNodeIds.push_back(node->id);
		}
		
		deferred.nodesToAdd.clear();
	}
	
	void removeNodesToRemove()
	{
		while (!deferred.nodesToRemove.empty())
		{
			auto nodeToRemoveItr = deferred.nodesToRemove.begin();
			auto nodeId = *nodeToRemoveItr;
			
			removeNodeSubTree(nodeId);
		}
	}
	
	bool undoCapture(std::string & text) const
	{
		LineWriter line_writer;
		
		if (scene.saveToLines(g_typeDB, line_writer, 0) == false)
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
	
	void undoCaptureBegin()
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
	
	void undoCaptureEnd()
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
	
	void undoReset()
	{
		undo.versions.clear();
		undo.currentVersion.clear();
		undo.currentVersionIndex = -1;
		
		undoCapture(undo.currentVersion);
		undo.versions.push_back(undo.currentVersion);
		undo.currentVersionIndex = undo.versions.size() - 1;
	}
	
	void performAction_undo()
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

				if (parseSceneFromLines(g_typeDB, lines, "", tempScene) == false)
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
						for (auto & node_itr : scene.nodes)
							deferred.nodesToRemove.insert(node_itr.second->id);
						
						removeNodesToRemove();
						
						scene = tempScene;
						
						undo.currentVersion = text;
						undo.currentVersionIndex--;
					}
				}
				
				tempScene.nodes.clear();
			}
		}
	}
	
	void performAction_redo()
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

				if (parseSceneFromLines(g_typeDB, lines, "", tempScene) == false)
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
						for (auto & node_itr : scene.nodes)
							deferred.nodesToRemove.insert(node_itr.second->id);
						
						removeNodesToRemove();
						
						scene = tempScene;
						
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
	
	void editNode(const int nodeId)
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
							
							if (doReflection_StructuredType(g_typeDB, *componentType, component, isSet, nullptr, &changedMemberObject))
							{
								// signal the component one of its properties has changed
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
						freeComponentInComponentSet(node.components, component);
					}
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
	
	void pasteNodeFromText(const int parentId, const char * text)
	{
		SceneNode * childNode = new SceneNode();
		childNode->id = scene.allocNodeId();
		childNode->parentId = parentId;
		
		if (node_from_text(text, *childNode) == false)
		{
			delete childNode;
			childNode = nullptr;
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
			}
			else
			{
				deferred.nodesToAdd.push_back(childNode);
			}
		}
	}
	
	void pasteNodeFromClipboard(const int parentId)
	{
		const char * text = SDL_GetClipboardText();
		
		if (text != nullptr)
		{
			pasteNodeFromText(parentId, text);
			
			SDL_free((void*)text);
			text = nullptr;
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

		auto * sceneNodeComponent = node.components.find<SceneNodeComponent>();
		
		char name[256];
		strcpy_s(name, sizeof(name), sceneNodeComponent->name.c_str());
		if (ImGui::InputText("Name", name, sizeof(name)))
		{
			sceneNodeComponent->name = name;
		}
		
		if (ImGui::MenuItem("Copy"))
		{
			result = kNodeStructureContextMenuResult_NodeCopy;
			
		// todo : copy nodes recursively
			std::string text;
			if (node_to_text(node, text))
			{
				SDL_SetClipboardText(text.c_str());
			}
		}
		
		if (ImGui::MenuItem("Paste as child", nullptr, false, SDL_HasClipboardText()))
		{
			result = kNodeStructureContextMenuResult_NodePaste;
			
		// todo : paste nodes recursively. and fixup parent-child id's
		// todo : update display names
			pasteNodeFromClipboard(node.id);
		}
		
		if (ImGui::MenuItem("Paste as sibling", nullptr, false, SDL_HasClipboardText()))
		{
			result = kNodeStructureContextMenuResult_NodePaste;
			
		// todo : paste nodes recursively. and fixup parent-child id's
		// todo : update display names
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
			
			auto * sceneNodeComponent = s_sceneNodeComponentMgr.createComponent(childNode->components.id);
			sceneNodeComponent->name = String::FormatC("Node %d", childNode->id);
			childNode->components.add(sceneNodeComponent);
			
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
	
	NodeContextMenuResult doNodeContextMenu(SceneNode & node, ComponentBase * selectedComponent)
	{
		//logDebug("context window for %d", node.id);
		
		NodeContextMenuResult result = kNodeContextMenuResult_None;
		
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
						
						auto * component = componentType->componentMgr->createComponent(node.components.id);
						
						if (component->init())
							node.components.add(component);
						else
							componentType->componentMgr->destroyComponent(node.components.id);
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
				nodeUi.nodesToOpen.count(node.id) != 0)
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
				for (auto & childNodeId : node.childNodeIds)
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
	
	void updateNodeVisibility()
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
			
			for (auto & nodeId : selectedNodes)
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
	
	void markNodeOpenUntilRoot(const int in_nodeId)
	{
		int nodeId = in_nodeId;

		while (nodeId != -1)
		{
			if (nodeUi.nodesToOpen.count(nodeId) != 0)
				break;
			
			nodeUi.nodesToOpen.insert(nodeId);
			
			auto & node = scene.getNode(nodeId);
			nodeId = node.parentId;
		}
	}
	
// todo : this is a test method. remove from scene editor and move elsewhere
	void addNodeFromTemplate_v1(Vec3Arg position, const AngleAxis & angleAxis, const int parentId)
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
	int addNodeFromTemplate_v2(Vec3Arg position, const AngleAxis & angleAxis, const int parentId)
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
		
		init_ok &= instantiateComponentsFromTemplate(g_typeDB, t, node->components);
		
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
		
		deferred.nodesToAdd.push_back(node);
		
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
								editNodeStructure_traverse(scene.rootNodeId);
								
								nodeUi.nodesToOpen.clear();
							}
							deferredEnd(true);
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
								deferredEnd(false);
							}
						}
					}
					
					// node editing
					
					if (ImGui::CollapsingHeader("Selected node(s)", ImGuiTreeNodeFlags_DefaultOpen))
					{
						deferredBegin();
						ImGui::BeginChild("Selected nodes", ImVec2(0, 300), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
						{
							for (auto & selectedNodeId : selectedNodes)
							{
								editNode(selectedNodeId);
							}
						}
						ImGui::EndChild();
						deferredEnd(false);
						
						// one or more transforms may have been edited or invalidated due to a parent node being invalidated
						// ensure the transforms are up to date by recalculating them. this is needed for transform gizmos
						// to work
						s_transformComponentMgr.calculateTransforms(scene);
					}
				}
				ImGui::End();
				
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
							performAction_copy();
						if (ImGui::MenuItem("Paste"))
							performAction_paste();
						ImGui::Separator();
						
						if (ImGui::MenuItem("Duplicate", nullptr, false, !selectedNodes.empty()))
							performAction_duplicate();
						
						ImGui::EndMenu();
					}
					
					if (ImGui::BeginMenu("Renderer"))
					{
						parameterUi::doParameterUi(renderer.parameterMgr, nullptr, false);
						
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
			
				guiContext.updateMouseCursor();
			}
			guiContext.processEnd();
		}
		
		// action: save
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_s) && keyboard.isDown(SDLK_LGUI))
		{
			inputIsCaptured = true;
			performAction_save();
		}
		
		// action: load
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_l) && keyboard.isDown(SDLK_LGUI))
		{
			inputIsCaptured = true;
			performAction_load();
		}
		
		// action: increase simulation speed
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_a) && keyboard.isDown(SDLK_LGUI))
		{
			inputIsCaptured = true;
			preview.tickMultiplier *= 1.25f;
		}
		
		// action: decrease simulation speed
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_z) && keyboard.isDown(SDLK_LGUI))
		{
			inputIsCaptured = true;
			preview.tickMultiplier /= 1.25f;
		}
		
		// action: copy selected node(s) to clipboard
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_c) && keyboard.isDown(SDLK_LGUI))
		{
			inputIsCaptured = true;
			performAction_copy();
		}
		
		// action: paste node(s) from clipboard
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_v) && keyboard.isDown(SDLK_LGUI))
		{
			inputIsCaptured = true;
			performAction_paste();
		}
		
		// action: duplicate selected node(s)
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_d) && keyboard.isDown(SDLK_LGUI))
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
			
			if (enablePadGizmo)
			{
				enablePadGizmo = false;
				
				if (inputIsCaptured == false)
				{
					transformGizmo.beginPad(cameraPosition, mouseDirection_world);
				}
			}
			
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
			//inputIsCaptured = true;
			
			// action: add node from template. todo : this is a test action. remove! instead, have a template list or something and allow adding instances from there
			if (keyboard.isDown(SDLK_RSHIFT))
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
							deferredEnd(true);
						}
						else
						{
							deferredBegin();
							{
								addNodeFromTemplate_v2(groundPosition, AngleAxis(), scene.rootNodeId);
							}
							deferredEnd(true);
						}
					}
				}
			}
			else
			{
				if (!keyboard.isDown(SDLK_LSHIFT))
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
			deferredEnd(false);
		}
		
		// update the camera
		
		if (camera.mode == Camera::kMode_FirstPerson)
			cameraIsActive = mouse.isDown(BUTTON_LEFT);
		else
			cameraIsActive = true;
		
		camera.tick(dt, inputIsCaptured, cameraIsActive == false);
		
		//
		
	#if defined(DEBUG)
		validateNodeReferences();
	#endif
	}
	
	void validateNodeReferences() const
	{
		for (auto & selectedNodeId : selectedNodes)
			Assert(scene.nodes.count(selectedNodeId) != 0);
		Assert(hoverNodeId == -1 || scene.nodes.count(hoverNodeId) != 0);
		Assert(nodeToGiveFocus == -1 || scene.nodes.count(nodeToGiveFocus) != 0);
		Assert(deferred.isFlushed());
	}
	
	void performAction_save()
	{
	#if ENABLE_SAVE_LOAD_TIMING
		auto t1 = SDL_GetTicks();
	#endif
		
		LineWriter line_writer;
		
		if (scene.saveToLines(g_typeDB, line_writer, 0) == false)
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
	
	void performAction_load()
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
	
	void performAction_copy()
	{
		if (selectedNodes.empty() == false)
		{
			auto nodeId = *selectedNodes.begin(); // todo : handle multiple nodes
			auto & node = scene.getNode(nodeId);
			std::string text;
			if (node_to_text(node, text))
				SDL_SetClipboardText(text.c_str());
		}
	}
	
	void performAction_paste()
	{
		deferredBegin();
		{
			pasteNodeFromClipboard(scene.rootNodeId);
		}
		deferredEnd(true);
	}
	
	void performAction_duplicate()
	{
		if (selectedNodes.empty() == false)
		{
			deferredBegin();
			{
				for (auto nodeId : selectedNodes)
				{
					auto & node = scene.getNode(nodeId);
					std::string text;
					if (node_to_text(node, text))
						pasteNodeFromText(scene.rootNodeId, text.c_str());
				}
			}
			deferredEnd(true);
		}
	}
	
	void drawNode(const SceneNode & node) const
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
	
	void drawNodes() const
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
	
	void drawSceneOpaque() const
	{
		s_modelComponentMgr.draw();
		
		if (visibility.drawGroundPlane)
		{
			setLumi(200);
			fillCube(Vec3(), Vec3(100, 1, 100));
		}
	}
	
	void drawEditorOpaque() const
	{
	#if ENABLE_TRANSFORM_GIZMOS
	// todo : draw transform gizmo on top of everything
		transformGizmo.draw();
	#endif
	}
	
	void drawOpaque() const
	{
		pushDepthTest(true, DEPTH_LEQUAL);
		pushBlend(BLEND_OPAQUE);
		{
			if (preview.drawScene)
			{
				drawSceneOpaque();
			}
		}
		popBlend();
		popDepthTest();
	}
	
	void drawSceneTranslucent() const
	{
	}
	
	void drawEditorTranslucent() const
	{
		if (visibility.drawGrid)
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
			{
				drawNodes();
			}
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
		int viewportSx = 0;
		int viewportSy = 0;
		framework.getCurrentViewportSize(viewportSx, viewportSy);
	
		Mat4x4 projectionMatrix;
		camera.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);
		
		Mat4x4 viewMatrix;
		camera.calculateViewMatrix(viewMatrix);
		
		renderer.draw(projectionMatrix, viewMatrix);
		
	#if 1
		// draw gizmos
		
	// todo : start a new render pass for this ?
		{
			gxMatrixMode(GX_PROJECTION);
			gxPushMatrix();
			gxLoadMatrixf(projectionMatrix.m_v);
			
			gxMatrixMode(GX_MODELVIEW);
			gxPushMatrix();
			gxLoadMatrixf(viewMatrix.m_v);
			{
				//gxClearDepth(1.f);
				
				pushDepthTest(false, DEPTH_LEQUAL);
				pushBlend(BLEND_OPAQUE);
				{
					drawEditorOpaque();
				}
				popBlend();
				popDepthTest();
			}
			gxMatrixMode(GX_PROJECTION);
			gxPopMatrix();
			
			gxMatrixMode(GX_MODELVIEW);
			gxPopMatrix();
		}
	#endif
		
		const_cast<SceneEditor*>(this)->guiContext.draw();
	}
	
	bool loadSceneFromLines_nonDestructive(std::vector<std::string> & lines, const char * basePath)
	{
		Scene tempScene;
		tempScene.createRootNode();

		if (parseSceneFromLines(g_typeDB, lines, basePath, tempScene) == false)
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
				deferredEnd(false);
				
				scene = tempScene;
				tempScene.nodes.clear();
				
				undoReset();
				
				return true;
			}
		}
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
	
	if (!parseTemplateFromFile("textfiles/resource-pointer-v1.txt", t))
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
	setupPaths(CHIBI_RESOURCE_PATHS);

	framework.enableRealTimeEditing = true;
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
	
	#if ENABLE_TEMPLATE_TEST
		if (inputIsCaptured == false && keyboard.wentDown(SDLK_t))
		{
			inputIsCaptured = true;
			
			// load scene description text file
	
			const char * path = "textfiles/scene-v1.txt";
			const std::string basePath = Path::GetDirectory(path);
			
			std::vector<std::string> lines;
			TextIO::LineEndings lineEndings;
			
			Scene tempScene;
			
			bool load_ok = true;
			
			if (!TextIO::load(path, lines, lineEndings))
			{
				logError("failed to load text file");
				load_ok = false;
			}
			else
			{
				editor.loadSceneFromLines_nonDestructive(lines, basePath.c_str());
			}
		}
	#endif
		
	#if ENABLE_PERFORMANCE_TEST
	// performance test. todo : remove this code
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
								int componentSetId = allocComponentSetId();
								
								LineWriter line_writer;
								object_tolines_recursive(g_typeDB, componentType, component, line_writer, 0);
								
								std::vector<std::string> lines = line_writer.to_lines();
								
								//for (auto & line : lines)
								//	logInfo("%s", line.c_str());
								
								auto * component_copy = componentType->componentMgr->createComponent(componentSetId);
								
								LineReader line_reader(lines, 0, 0);
								if (object_fromlines_recursive(g_typeDB, componentType, component_copy, line_reader))
								{
									//logDebug("success!");
								}
								
								componentType->componentMgr->destroyComponent(componentSetId);
								component_copy = nullptr;
								
								freeComponentSetId(componentSetId);
								Assert(componentSetId == kComponentSetIdInvalid);
							}
							
							auto t2 = SDL_GetTicks();
							printf("time: %ums\n", (t2 - t1));
							printf("(done)\n");
						}
					}
				}
			}
		}
	#endif
	
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
