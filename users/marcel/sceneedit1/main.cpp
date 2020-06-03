// component types
#include "components/lightComponent.h"
#include "components/transformComponent.h"
#include "scene/sceneNodeComponent.h"

// ecs-sceneEditor
#include "editor/sceneEditor.h"

// ecs-scene
#include "helpers.h" // g_componentTypes

// ecs-component
#include "componentType.h"

// ecs-parameter
#include "parameter.h"
#include "parameterUi.h"

// libreflection
#include "lineReader.h"
#include "lineWriter.h"
#include "reflection-textio.h"

// sceneedit
#include "helpers2.h" // g_typeDB

// framework
#include "framework.h"
#include "gx_render.h"

// libgg
#include "Path.h"
#include "TextIO.h"

#if defined(DEBUG)
	#define ENABLE_PERFORMANCE_TEST     1
	#define ENABLE_TEMPLATE_TEST        1
#else
	#define ENABLE_PERFORMANCE_TEST     0 // do not alter
	#define ENABLE_TEMPLATE_TEST        0 // do not alter
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

static const int VIEW_SX = 1200;
static const int VIEW_SY = 800;

// todo : move these instances to helpers.cpp
extern TransformComponentMgr s_transformComponentMgr;
extern LightComponentMgr s_lightComponentMgr;
extern ParameterComponentMgr s_parameterComponentMgr;

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
	testResourcePointers();
	return 0;
#endif
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	registerBuiltinTypes(g_typeDB);
	registerComponentTypes(g_typeDB);
	
	initComponentMgrs();
	
	testResources(); // todo : remove
	
	SceneEditor editor;
	editor.init(&g_typeDB);
	
	auto drawOpaque = [&]()
		{
			pushDepthTest(true, DEPTH_LEQUAL);
			pushBlend(BLEND_OPAQUE);
			{
				editor.drawSceneOpaque();
				editor.drawEditorOpaque();
			}
			popBlend();
			popDepthTest();
		};
	
	auto drawTranslucent = [&]()
		{
			pushDepthTest(true, DEPTH_LESS, false);
			pushBlend(BLEND_ALPHA);
			{
				editor.drawSceneTranslucent();
				editor.drawEditorTranslucent();
			}
			popBlend();
			popDepthTest();
			
			pushDepthTest(false, DEPTH_LESS);
			pushBlend(BLEND_ALPHA);
			{
				editor.drawEditorGizmos();
			}
			popBlend();
			popDepthTest();
		};

	Renderer renderer;
	renderer.init();
	renderer.drawOpaque = [&]() { drawOpaque(); };
	renderer.drawTranslucent = [&]() { drawTranslucent(); };
	
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
		
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Renderer"))
			{
				parameterUi::doParameterUi(renderer.parameterMgr, nullptr, false);
				
				ImGui::EndMenu();
			}
			
			ImGui::EndMainMenuBar();
		}
	
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
			int viewportSx = 0;
			int viewportSy = 0;
			framework.getCurrentViewportSize(viewportSx, viewportSy);

			Mat4x4 projectionMatrix;
			editor.camera.calculateProjectionMatrix(viewportSx, viewportSy, projectionMatrix);
			
			Mat4x4 viewMatrix;
			editor.camera.calculateViewMatrix(viewMatrix);
			
			renderer.draw(projectionMatrix, viewMatrix);
			
			projectScreen2d();

			editor.drawEditor();
		}
		framework.endDraw();
	}
	
	editor.shut();
	
	// todo : shutdown renderer
	
	shutComponentMgrs();
	
	framework.shutdown();

	return 0;
}
