// component types
#include "components/gltfComponent.h"
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

// ecs-system-audio
#include "audioEmitterToAudioOutput.h" // todo : rename file to StereoOutputBuffer
#include "audiooutput/AudioOutput_Native.h" // todo : remove
#include <mutex> // todo : remove

// libreflection
#include "lineReader.h"
#include "lineWriter.h"
#include "reflection-textio.h"

// sceneedit
#include "helpers2.h" // registerBuiltinTypes, registerComponentTypes

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

*/

static const int VIEW_SX = 1000;
static const int VIEW_SY = 800;

//

struct MyRenderOptions
{
	enum Mode
	{
		kMode_Flat,
		kMode_DeferredShaded,
		kMode_DeferredShadedWithShadows,
		kMode_ForwardShaded,
		kMode_ForwardShadedWithShadows
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
				{ "Deferred Shaded", kMode_DeferredShaded },
				{ "Deferred Shaded + Shadows", kMode_DeferredShadedWithShadows },
				{ "Forward Shaded", kMode_ForwardShaded },
				{ "Forward Shaded + Shadows", kMode_ForwardShadedWithShadows }
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
	registerBuiltinTypes(typeDB);
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

#if FRAMEWORK_IS_NATIVE_VR || 1 // todo : decide what to do here
	framework.vrMode = true;
	framework.enableVrMovement = true;
#endif

	framework.enableRealTimeEditing = true;
	framework.enableDepthBuffer = true;
	framework.allowHighDpi = true;
	framework.msaaLevel = 4;
	
	framework.windowIsResizable = true;
	
#if USE_GUI_WINDOW
	framework.windowX = 450;
	framework.windowY = 100;
#endif
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;
	
	TypeDB typeDB;

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
	guiWindow->setPosition(40, 100);
#endif
	
	auto drawOpaque = [&]()
		{
			pushCullMode(CULL_BACK, CULL_CCW);
			{
				// draw scene
				
				pushWireframe(myRenderOptions.drawWireframe->get());
				{
					editor.drawSceneOpaque();
				}
				popWireframe();
				
				// draw editor
				
				editor.drawView3dOpaque();
			}
			popCullMode();
		};
	
	auto drawOpaque_ForwardShaded = [&]()
		{
			pushCullMode(CULL_BACK, CULL_CCW);
			{
				// draw scene
				
				pushWireframe(myRenderOptions.drawWireframe->get());
				{
					editor.drawSceneOpaque_ForwardShaded();
				}
				popWireframe();
				
				// draw editor
				
				editor.drawView3dOpaque_ForwardShaded();
				
				// draw virtual desktop
				
				if (framework.vrMode)
				{
					framework.drawVirtualDesktop();
				}
			}
			popCullMode();
		};
		
	auto drawTranslucent = [&]()
		{
			pushCullMode(CULL_BACK, CULL_CCW);
			pushBlend(BLEND_ALPHA);
			{
				// draw scene
				
				pushWireframe(myRenderOptions.drawWireframe->get());
				{
					editor.drawSceneTranslucent();
				}
				popWireframe();
				
				// draw editor
				
				editor.drawView3dTranslucent();
			}
			popBlend();
			popCullMode();
		};

	auto drawLights = [&]()
		{
			// draw lights using depth and normal maps as inputs
			
			for (auto * light = g_lightComponentMgr.head; light != nullptr; light = light->next)
			{
				if (light->enabled == false)
					continue;
				
				// note : the deferred light drawing functions will convert the light color to the linear color space for us
				
				if (light->type == LightComponent::kLightType_Directional)
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
				else if (light->type == LightComponent::kLightType_Point)
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
						light->farDistance * light->attenuationBegin,
						light->farDistance,
						light->color,
						light->intensity);
				}
				else if (light->type == LightComponent::kLightType_Spot)
				{
					// draw spot light
					
					auto * sceneNode = light->componentSet->find<SceneNodeComponent>();
					
					Vec3 lightPosition_world;
					Vec3 lightDirection_world(0, 0, 1);
					
					if (sceneNode != nullptr)
					{
						lightPosition_world = sceneNode->objectToWorld.GetTranslation();
						lightDirection_world = sceneNode->objectToWorld.GetAxis(2).CalcNormalized();
					}
					
					rOne::g_lightDrawer.drawDeferredSpotLight(
						lightPosition_world,
						lightDirection_world,
						light->spotAngle / 180.f * float(M_PI),
						light->farDistance * light->attenuationBegin,
						light->farDistance,
						light->color,
						light->intensity);
				}
			}
		};
		
	rOne::Renderer renderer;
	renderer.registerShaderOutputs();
	
	rOne::RenderFunctions renderFunctions;
	renderFunctions.drawOpaque = drawOpaque;
	renderFunctions.drawOpaque_ForwardShaded = drawOpaque_ForwardShaded;
	renderFunctions.drawTranslucent = drawTranslucent;
	renderFunctions.drawLights = drawLights;
	
	rOne::RenderOptions renderOptions;
	if (framework.isStereoVr() == false)
	{
		renderOptions.bloom.enabled = true;
		renderOptions.motionBlur.enabled = true;
		renderOptions.depthSilhouette.enabled = true;
		renderOptions.lightScatter.enabled = true;
		renderOptions.linearColorSpace = true;
	}
	
#if USE_GUI_WINDOW
	int framesSinceIdleForGuiWindow = 0;
#endif
	int framesSinceIdleForView3dWindow = 0;
	
#if USE_GUI_WINDOW && WINDOW_IS_3D
	Mat4x4 guiWindowTransform(true);
#endif

	FpsCounter fpsCounter;
	
// todo : remove this test code
	class MyAudioStream : public AudioStream
	{
	public:
		std::mutex mutex;
		
		Mat4x4 worldToView;
		
		virtual int Provide(int numSamples, AudioSample * __restrict samples) override
		{
			Mat4x4 worldToView_local;
			
			mutex.lock();
			{
				worldToView_local = worldToView;
			}
			mutex.unlock();
			
			float outputBufferL[numSamples];
			float outputBufferR[numSamples];
			audioEmitterToStereoOutputBuffer(
				worldToView_local,
				outputBufferL,
				outputBufferR,
				numSamples);
			
			const float scale = ((1 << 15) - 1);
			
			for (int i = 0; i < numSamples; ++i)
			{
				samples[i].channel[0] = int(outputBufferL[i] * scale);
				samples[i].channel[1] = int(outputBufferR[i] * scale);
			}
			
			return numSamples;
		}
	};
	AudioOutput_Native audioOutput;
	audioOutput.Initialize(2, 48000, 256);
	MyAudioStream audioStream;
	audioOutput.Play(&audioStream);
	
	for (;;)
	{
		framework.waitForEvents =
		#if USE_GUI_WINDOW
			framesSinceIdleForGuiWindow >= 3 &&
		#endif
			framesSinceIdleForView3dWindow >= 3;
			
		framework.process();
		
		if (keyboard.wentDown(SDLK_ESCAPE))
			framework.quitRequested = true;
		
		if (framework.quitRequested)
			break;

		if (framework.waitForEvents)
			framework.timeStep = 0.f;
			
		const float dt = framework.timeStep;
		
		bool inputIsCaptured = false;
		
		uiCaptureBeginFrame(inputIsCaptured);
		
	#if USE_GUI_WINDOW && WINDOW_IS_3D
		if (vrPointer[1].isDown(VrButton_GripTrigger))
		{
			guiWindowTransform = vrPointer[1].getTransform(Vec3());
		}
		
		guiWindow->setTransform(
			Mat4x4(true)
			.Translate(framework.vrOrigin)
			.Mul(guiWindowTransform));
	#endif
	
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
		bool redrawGui = false;
	#endif
		bool redrawView3d = false;
		
	#if USE_GUI_WINDOW
		pushWindow(*guiWindow);
	#endif
		{
		#if USE_GUI_WINDOW
			// note : we track the input capture separately when using a separate window for the gui
			bool inputIsCaptured = false;
			
			inputIsCaptured |= guiWindow->hasFocus() == false;
		#endif
			
			editor.tickGui(dt, inputIsCaptured);
			
			if (!mouse.isIdle() || !keyboard.isIdle())
				redrawView3d = true;
				
		#if !USE_GUI_WINDOW
			redrawView3d |= !editor.guiContext.isIdle();
		#endif
		
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
			
		#if USE_GUI_WINDOW
			framesSinceIdleForGuiWindow++;
			if (!mouse.isIdle() || !keyboard.isIdle() || !editor.guiContext.isIdle())
				framesSinceIdleForGuiWindow = 0;
			
			if (framesSinceIdleForGuiWindow < 3)
			{
				framework.beginDraw(0, 0, 0, 0);
				{
					editor.drawGui();
				}
				framework.endDraw();
			}
			else
			{
				editor.guiContext.skipDraw();
			}
		#endif
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
			editor.preview.viewportX = 0;
			editor.preview.viewportY = 0;
			if (editor.showUi)
			{
				editor.preview.viewportX = SceneEditor::kMainWindowWidth;
				editor.preview.viewportSx -= SceneEditor::kMainWindowWidth;
			}
		}
		
		// tick editor viewport
		
		editor.tickView(dt, inputIsCaptured);
	
		if (!mouse.isIdle() || !keyboard.isIdle())
			redrawView3d = true;
			
		//
		
		g_transformComponentMgr.calculateTransforms(editor.scene);
		
		if (editor.preview.tickScene)
		{
			editor.undoCaptureBegin(false); // may or may not be pristine. as something may currently be in the process of being edited
			{
				const float dt_scene = dt * editor.preview.tickMultiplier;
				
				for (auto * type : g_componentTypeDB.componentTypes)
				{
					type->componentMgr->tick(dt_scene);
				}
			}
			editor.undoCaptureFastForward();
			
		// todo : why do we calculate transforms again here ? if there is a reason, add a comment
			g_transformComponentMgr.calculateTransforms(editor.scene);
			
			redrawView3d = true;
		}
		
		//
		
		if (framework.isStereoVr())
		{
			redrawView3d = true;
		}
		
		//
		
		framesSinceIdleForView3dWindow++;
		if (redrawView3d)
			framesSinceIdleForView3dWindow = 0;
			
		if (framesSinceIdleForView3dWindow < 3)
		{
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
					
					// prepare lights
					
					Mat4x4 worldToView;
					gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
					g_lightComponentMgr.beforeDraw(worldToView);

					// todo : remove this test code
					audioStream.mutex.lock();
					{
						audioStream.worldToView = worldToView;
					}
					audioStream.mutex.unlock();
					
					// render the scene
					
				// todo : perhaps add a SceneRenderComponentMgr which will render the scene for us (on request). forward lighting helper and shadow map drawer could live there
				
					switch (myRenderOptions.mode->get())
					{
					case MyRenderOptions::kMode_Flat:
						renderOptions.renderMode = rOne::kRenderMode_Flat;
						g_lightComponentMgr.enableShadowMaps = false;
						g_gltfComponentMgr.enableForwardShading = true;
						break;
						
					case MyRenderOptions::kMode_DeferredShaded:
						renderOptions.renderMode = rOne::kRenderMode_DeferredShaded;
						g_lightComponentMgr.enableShadowMaps = false;
						g_gltfComponentMgr.enableForwardShading = false;
						break;
						
					case MyRenderOptions::kMode_DeferredShadedWithShadows:
						renderOptions.renderMode = rOne::kRenderMode_DeferredShaded;
						g_lightComponentMgr.enableShadowMaps = true;
						g_gltfComponentMgr.enableForwardShading = false;
						break;
					
					case MyRenderOptions::kMode_ForwardShaded:
						renderOptions.renderMode = rOne::kRenderMode_ForwardShaded;
						g_lightComponentMgr.enableShadowMaps = false;
						g_gltfComponentMgr.enableForwardShading = true;
						break;
						
					case MyRenderOptions::kMode_ForwardShadedWithShadows:
						renderOptions.renderMode = rOne::kRenderMode_ForwardShaded;
						g_lightComponentMgr.enableShadowMaps = true;
						g_gltfComponentMgr.enableForwardShading = true;
						break;
					}
					
					renderOptions.drawNormals = myRenderOptions.drawNormals->get();
					
					renderer.render(renderFunctions, renderOptions, framework.timeStep);
					
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
		}
		else
		{
		#if !USE_GUI_WINDOW
			editor.guiContext.skipDraw();
		#endif
		}

		if (framework.vrMode)
		{
			framework.present();
		}
		
		static int n = 0; // todo : remove fps counter
		++n;
		static int f = 0;
		if ((int)framework.time != f)
		{
			logInfo("fps: %d", n / framework.getEyeCount());
			n = 0;
			f = (int)framework.time;
		}
	}
	
	renderer.free();
	
#if USE_GUI_WINDOW
	delete guiWindow;
	guiWindow = nullptr;
#endif

	editor.shut();
	
	g_componentTypeDB.shutComponentMgrs();
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
