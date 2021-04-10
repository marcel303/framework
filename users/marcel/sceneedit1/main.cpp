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
#include "componentTypeDB.h"

// ecs-parameter
#include "parameter.h"
#include "parameterUi.h"

// libreflection
#include "lineReader.h"
#include "lineWriter.h"
#include "reflection-textio.h"

// sceneedit
#include "helpers2.h" // g_typeDB

// renderOne
#include "lightDrawer.h"
#include "renderer.h"

// ui
#include "editor/ui-capture.h"

// framework
#include "framework.h"
#include "gx_render.h"

// libgg
#include "Path.h"
#include "TextIO.h"
#include "Timer.h"

#include "components/gltfCache.h" // todo : remove

#if defined(DEBUG)
	#define ENABLE_PERFORMANCE_TEST     1
	#define ENABLE_TEMPLATE_TEST        1
#else
	#define ENABLE_PERFORMANCE_TEST     0 // do not alter
	#define ENABLE_TEMPLATE_TEST        0 // do not alter
#endif

#if defined(ANDROID)
	#define USE_GUI_WINDOW 1
#else
	#define USE_GUI_WINDOW 1
#endif

/*

todo :

- add template file list / browser
- add support for template editing
- add option to add nodes from template

done :

+ add lookat/focus option to scene structure view
	+ as a context menu
+ add 'Paste tree' to scene structure context menu
+ add support for copying node tree 'Copy tree'
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

//

struct MyRenderOptions
{
	enum Mode
	{
		kMode_Flat,
		kMode_Lit,
		kMode_LitWithShadows
	};
	
	ParameterMgr parameterMgr;
	
	ParameterEnum * mode = nullptr;
	ParameterBool * drawWireframe = nullptr;
	ParameterBool * drawNormals = nullptr;
	
	MyRenderOptions()
	{
		parameterMgr.init("renderer");
		
		mode = parameterMgr.addEnum("Mode", kMode_Flat,
			{
				{ "Flat", kMode_Flat },
				{ "Lit", kMode_Lit },
				{ "Lit + Shadows", kMode_LitWithShadows }
			});
		
		drawWireframe = parameterMgr.addBool("Draw Wireframe", false);
		drawNormals = parameterMgr.addBool("Draw Normals", false);
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

static bool testResourcePointers(const TypeDB & typeDB)
{
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
		
		instantiateComponentsFromTemplate(typeDB, t, componentSet);
		
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

#if FRAMEWORK_IS_NATIVE_VR || 1
	framework.vrMode = true;
	framework.enableVrMovement = true;
#endif

	framework.enableRealTimeEditing = true;
	framework.enableDepthBuffer = true;
	framework.allowHighDpi = false;
	
	framework.windowIsResizable = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	auto & typeDB = g_typeDB;

	registerBuiltinTypes(typeDB);
	registerComponentTypes(typeDB, g_componentTypeDB);
	
	g_componentTypeDB.initComponentMgrs();
	
	testResources(); // todo : remove
	
#if 0
	testResourcePointers(typeDB); // todo : remove
#endif
	
	SceneEditor editor;
	editor.init(&typeDB);
	
	MyRenderOptions myRenderOptions;
	
#if USE_GUI_WINDOW
	Window * guiWindow = new Window("Gui", 400, 600, true);
#endif
	
	auto drawOpaque = [&]()
		{
			pushCullMode(CULL_BACK, CULL_CCW);
			{
				if (myRenderOptions.drawWireframe->get())
					pushWireframe(true);
				{
					editor.drawSceneOpaque();
				}
				if (myRenderOptions.drawWireframe->get())
					popWireframe();
				
				editor.drawEditorOpaque();
				
				pushCullMode(CULL_BACK, CULL_CCW);
				editor.drawEditorGizmosOpaque(false);
				popCullMode();
				
				pushDepthTest(true, DEPTH_GREATER, false);
				{
					pushCullMode(CULL_BACK, CULL_CCW);
					editor.drawEditorGizmosOpaque(true);
					popCullMode();
				}
				popDepthTest();

				if (framework.vrMode)
				{
					framework.drawVirtualDesktop();
				}
			}
			popCullMode();
		};
	
	auto drawOpaqueUnlit = [&]()
		{
			editor.drawView3d();
		};
		
	auto drawTranslucent = [&]()
		{
			pushBlend(BLEND_ALPHA);
			{
				editor.drawSceneTranslucent();
				editor.drawEditorTranslucent();
				editor.drawEditorGizmosTranslucent();
			}
			popBlend();
		};

	auto drawLights = [&]()
		{
			// draw lights using depth and normal maps as inputs
			
			for (auto * light = g_lightComponentMgr.head; light != nullptr; light = light->next)
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
					
					rOne::g_lightDrawer.drawDeferredPointLight(
						lightPosition_world,
						light->innerRadius,
						light->outerRadius,
						light->color,
						light->intensity);
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
					
					rOne::g_lightDrawer.drawDeferredDirectionalLight(
						lightDir_world.CalcNormalized(),
						light->color,
						light->bottomColor,
						light->intensity);
				}
			}
		};
		
	rOne::Renderer renderer;
	renderer.registerShaderOutputs();
	
	rOne::RenderFunctions renderFunctions;
	renderFunctions.drawOpaque = drawOpaque;
	renderFunctions.drawOpaqueUnlit = drawOpaqueUnlit;
	renderFunctions.drawTranslucent = drawTranslucent;
	renderFunctions.drawLights = drawLights;
	
	rOne::RenderOptions renderOptions;
	renderOptions.deferredLighting.enableStencilVolumes = false;
	renderOptions.bloom.enabled = true;
	//renderOptions.motionBlur.enabled = true;
	//renderOptions.depthSilhouette.enabled = true;
	renderOptions.lightScatter.enabled = true;
	renderOptions.linearColorSpace = true;
	
	for (;;)
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;

		const float dt = framework.timeStep;
		
		bool inputIsCaptured = false;
		
		uiCaptureBeginFrame(inputIsCaptured);
		
		if (framework.vrMode)
		{
			const Mat4x4 transform = vrPointer[0].getTransform(framework.vrOrigin);
			const bool transformIsValud = vrPointer[0].hasTransform;
			
			const int buttonMask =
				vrPointer[0].isDown(VrButton_Trigger) << 0 |
				vrPointer[0].isDown(VrButton_GripTrigger) << 1;
			
			inputIsCaptured |= framework.tickVirtualDesktop(
				transform,
				transformIsValud,
				buttonMask,
				false);
		}
		
	#if USE_GUI_WINDOW
		pushWindow(*guiWindow);
	#endif
		{
		#if USE_GUI_WINDOW
			// note : we track the input capture separately when using a separate window for the gui
			bool inputIsCaptured = false;
		#endif
			
			editor.tickGui(dt, inputIsCaptured);
			
		// todo : active gui context.. ?
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("Renderer"))
				{
				#if 1 // todo : add libreflection-imgui which enables editing reflected types through a gui
					parameterUi::doParameterUi(myRenderOptions.parameterMgr, nullptr, false);
				#endif
				
					ImGui::EndMenu();
				}
				
				ImGui::EndMainMenuBar();
			}
		
			if (ImGui::BeginMainMenuBar())
			{
				if (ImGui::BeginMenu("Tests"))
				{
				#if ENABLE_TEMPLATE_TEST
					if (ImGui::MenuItem("Template load test"))
					{
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
					if (ImGui::MenuItem("Save & load performance test"))
					{
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
									auto * componentType = g_componentTypeDB.findComponentType(component->typeIndex());
									
									Assert(componentType != nullptr);
									if (componentType != nullptr)
									{
										auto t1 = g_TimerRT.TimeUS_get();
										
										const int numIterations = 100000;
										
										for (int i = 0; i < numIterations; ++i)
										{
											int componentSetId = allocComponentSetId();
											
											LineWriter line_writer;
											object_tolines_recursive(typeDB, componentType, component, line_writer, 0);
											
											std::vector<std::string> lines = line_writer.to_lines();
											
											//for (auto & line : lines)
											//	logInfo("%s", line.c_str());
											
											auto * component_copy = componentType->componentMgr->createComponent(componentSetId);
											
											LineReader line_reader(lines, 0, 0);
											if (object_fromlines_recursive(typeDB, componentType, component_copy, line_reader))
											{
												//logDebug("success!");
											}
											
											componentType->componentMgr->destroyComponent(componentSetId);
											component_copy = nullptr;
											
											freeComponentSetId(componentSetId);
											Assert(componentSetId == kComponentSetIdInvalid);
										}

										auto t2 = g_TimerRT.TimeUS_get();
										logDebug("save & load test took: %ums for %d iterations", (t2 - t1) / 1000, numIterations);
										logDebug("(done)");
									}
								}
							}
						}
					}
				#endif
					
					ImGui::EndMenu();
				}
				
				ImGui::EndMainMenuBar();
			}
		}
	#if USE_GUI_WINDOW
		popWindow();
	#endif
	
		// update editor viewport for mouse picking
		
		if (framework.isStereoVr())
		{
			editor.preview.viewportX = 0;
			editor.preview.viewportY = 0;
			framework.getCurrentViewportSize(
				editor.preview.viewportSx,
				editor.preview.viewportSy);
		}
		else if (USE_GUI_WINDOW)
		{
			editor.preview.viewportX = 0;
			editor.preview.viewportY = 0;
			framework.getCurrentViewportSize(
				editor.preview.viewportSx,
				editor.preview.viewportSy);
		}
		else
		{
		// todo : add SceneEditor::drawPreview, when true, let the editor show a preview window
		// todo : add SceneEditor::previewTextureId, and draw it before the editor draws itself
			framework.getCurrentViewportSize(
				editor.preview.viewportSx,
				editor.preview.viewportSy);
			editor.preview.viewportX = SceneEditor::kMainWindowWidth;
			editor.preview.viewportSx -= SceneEditor::kMainWindowWidth;
		}
		
		// tick editor viewport
		
		editor.tickView(dt, inputIsCaptured);
	
		//
		
		g_transformComponentMgr.calculateTransforms(editor.scene);
		
		if (editor.preview.tickScene)
		{
			const float dt_scene = dt * editor.preview.tickMultiplier;
			
			for (auto * type : g_componentTypeDB.componentTypes)
			{
				type->componentMgr->tick(dt_scene);
			}
			
			g_transformComponentMgr.calculateTransforms(editor.scene);
		}
		
		//
		
	#if USE_GUI_WINDOW
	#if WINDOW_IS_3D
		if (vrPointer[1].isDown(VrButton_GripTrigger))
		{
			guiWindow->setTransform(
				vrPointer[1].getTransform(framework.vrOrigin));
		}
	#endif

		pushWindow(*guiWindow);
		{
			framework.beginDraw(0, 0, 0, 0);
			{
				editor.drawGui();
			}
			framework.endDraw();
		}
		popWindow();
	#endif
		
		//
		
		for (int i = 0; i < framework.getEyeCount(); ++i)
		{
			framework.beginEye(i, colorBlack);
			{
				if (!framework.isStereoVr())
				{
					// set projection and view matrices based on editor camera
					
					if (framework.vrMode)
					{
						// reset the view matrix
						Mat4x4 identity(true);
						gxSetMatrixf(GX_MODELVIEW, identity.m_v);
					}
					
					editor.camera.pushProjectionMatrix();
					editor.camera.pushViewMatrix();
					
					{
						// hack to apply viewport origin
						
						Mat4x4 projectionMatrix;
						gxGetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
						int viewSx;
						int viewSy;
						framework.getCurrentViewportSize(viewSx, viewSy);
						projectionMatrix = Mat4x4(true)
							.Translate(
								editor.preview.viewportX / float(viewSx),
								editor.preview.viewportY / float(viewSy), 0)
							.Mul(projectionMatrix);
						gxSetMatrixf(GX_PROJECTION, projectionMatrix.m_v);
					}
				}
				
				// render the scene
				
				switch (myRenderOptions.mode->get())
				{
				case MyRenderOptions::kMode_Flat:
					renderOptions.renderMode = rOne::kRenderMode_Flat;
					break;
					
				case MyRenderOptions::kMode_Lit:
				case MyRenderOptions::kMode_LitWithShadows:
					//renderOptions.renderMode = rOne::kRenderMode_ForwardShaded; // todo : switch to forward shaded
					renderOptions.renderMode = rOne::kRenderMode_DeferredShaded;
					break;
				}
				
				renderOptions.drawNormals = myRenderOptions.drawNormals->get();
				
				renderer.render(renderFunctions, renderOptions, framework.timeStep);
				
				static int n = 0;
				++n;
				static int f = 0;
				if ((int)framework.time != f)
				{
					logInfo("fps: %d", n / framework.getEyeCount());
					n = 0;
					f = (int)framework.time;
				}
				
				if (!framework.isStereoVr())
				{
					// restore projection and view matrices
					
					editor.camera.popViewMatrix();
					editor.camera.popProjectionMatrix();
				}
				
				if (!framework.isStereoVr())
				{
					// draw 2D gui
					
					projectScreen2d();

					editor.drawView2d();
				}

			#if !USE_GUI_WINDOW
				editor.drawGui();
			#endif
			}
			framework.endEye();
		}

		if (framework.vrMode)
		{
			framework.present();
		}
	}
	
	renderer.free();
	
#if USE_GUI_WINDOW
	delete guiWindow;
	guiWindow = nullptr;
#endif

	editor.shut();
	
	g_componentTypeDB.shutComponentMgrs();
	
	g_gltfCache.clear(); // todo : move to framework?
	
	framework.shutdown();

	return 0;
}
