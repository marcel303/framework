#include "framework.h"
#include "gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"
#include "Quat.h"

#define VIEW_SX 1000
#define VIEW_SY 600

#define USE_BUFFER_CACHE 1 // create static vertex and index buffers from gltf resources

#define ANIMATED_CAMERA 0 // todo : remove option and use hybrid

#if ANIMATED_CAMERA

struct AnimatedCamera3d
{
	Vec3 position;
	Vec3 lookatTarget;
	
	Vec3 desiredPosition;
	Vec3 desiredLookatTarget;
	bool animate = false;
	float animationSpeed = 1.f;
	
	void tick(const float dt, const bool inputIsCaptured)
	{
		if (inputIsCaptured == false)
		{
		
		}
		
		if (animate)
		{
			const float retain = powf(1.f - animationSpeed, dt);
			const float attain = 1.f - retain;
			
			position = lerp(position, desiredPosition, attain);
			lookatTarget = lerp(lookatTarget, desiredLookatTarget, attain);
		}
	}
	
	Mat4x4 getWorldMatrix() const
	{
		return getViewMatrix().CalcInv();
	}

	Mat4x4 getViewMatrix() const
	{
		Mat4x4 m;
		m.MakeLookat(position, lookatTarget, Vec3(0, 1, 0));
		return m;
	}

	void pushViewMatrix() const
	{
		const Mat4x4 matrix = getViewMatrix();
		
		const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
		{
			gxMatrixMode(GX_PROJECTION);
			gxPushMatrix();
			gxMultMatrixf(matrix.m_v);
		}
		gxMatrixMode(restoreMatrixMode);
	}

	void popViewMatrix() const
	{
		const GX_MATRIX restoreMatrixMode = gxGetMatrixMode();
		{
			gxMatrixMode(GX_PROJECTION);
			gxPopMatrix();
		}
		gxMatrixMode(restoreMatrixMode);
	}
};

#endif

#include "zippyDownload.h"

#define MEDIA_BASE_URL "https://centuryofthecat.nl/shared_media/framework/framework-gltf/"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	framework.msaaLevel = 4;
	framework.filedrop = true;
	framework.allowHighDpi = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	//const char * name = "van_gogh_room";
	const char * name = "littlest_tokyo";
	//const char * name = "ftm";
	//const char * name = "nara_the_desert_dancer";
	//const char * name = "drone";
	//const char * name = "buster_drone";
	//const char * name = "halloween_little_witch";
	//const char * name = "kalestra_the_sorceress";
	
	auto url = std::string(MEDIA_BASE_URL) + name + ".zip";
	
	zippyDownload(url.c_str(), "scenes");
	
	auto path = std::string("scenes/") + name + "/scene.gltf";
	
	gltf::Scene scene;
	
	if (!gltf::loadScene(path.c_str(), scene))
	{
		logError("failed to load GLTF file");
	}
	
#if USE_BUFFER_CACHE
	gltf::BufferCache * bufferCache = new gltf::BufferCache();
	bufferCache->init(scene);
#else
	gltf::BufferCache * bufferCache = nullptr;
#endif
	
#if ANIMATED_CAMERA
	AnimatedCamera3d camera;
#else
	Camera3d camera;
#endif
	
	camera.position = Vec3(0, 0, -2);
	
	bool centimeters = true;
	bool useAlphaToCoverage = false;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;
		
		for (auto & file : framework.droppedFiles)
		{
			delete bufferCache;
			bufferCache = nullptr;
			
			scene = gltf::Scene();
			
			if (gltf::loadScene(file.c_str(), scene))
			{
			#if USE_BUFFER_CACHE
				bufferCache = new gltf::BufferCache();
				bufferCache->init(scene);
			#endif
			}
		}
		
		if (keyboard.wentDown(SDLK_c))
			centimeters = !centimeters;
		if (keyboard.wentDown(SDLK_a))
			useAlphaToCoverage = !useAlphaToCoverage;
		
	#if ANIMATED_CAMERA
		if (keyboard.wentDown(SDLK_p))
		{
			gltf::BoundingBox boundingBox;
			
			calculateSceneMinMax(scene, boundingBox);
			
			if (centimeters)
			{
				boundingBox.min[0] = -boundingBox.min[0];
				boundingBox.max[0] = -boundingBox.max[0];
				
				boundingBox.min *= .01f;
				boundingBox.max *= .01f;
			}
			
			const float distance = (boundingBox.max - boundingBox.min).CalcSize() / 2.f * .9f;
			const Vec3 target = (boundingBox.min + boundingBox.max) / 2.f;
			
			const float angle = random<float>(-M_PI, +M_PI);
			camera.desiredPosition = target + Mat4x4(true).RotateY(angle).GetAxis(2) * distance;
			camera.desiredLookatTarget = target;
			camera.animate = true;
			camera.animationSpeed = .9f;
		}
	#endif
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			const float scaleMultiplier = centimeters ? .01f : 1.f;
			
			projectPerspective3d(60.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			pushBlend(BLEND_OPAQUE);
			camera.pushViewMatrix();
			{
				gxScalef(scaleMultiplier, scaleMultiplier, scaleMultiplier);
				
				Mat4x4 viewMatrix;
				gxGetMatrixf(GX_MODELVIEW, viewMatrix.m_v);
				
				// render passes:
				// 0: depth pre-pass (to avoid overdraw)
				// 1: opaque pass
				// 2: translucent pass
				for (int i = 0; i < 3; ++i)
				{
					const bool isDepthPrePass = (i == 0);
					const bool isOpaquePass = (i == 0 || i == 1);
					
					const BLEND_MODE blendMode =
						useAlphaToCoverage
							? BLEND_OPAQUE
							: (isOpaquePass ? BLEND_OPAQUE : BLEND_ALPHA);
					
					pushShaderOutputs(isDepthPrePass ? "" : "c");
					pushDepthTest(true,
						isDepthPrePass ? DEPTH_LESS :
						isOpaquePass ? DEPTH_LEQUAL :
							DEPTH_LESS,
						blendMode == BLEND_OPAQUE);
					pushBlend(blendMode);
					{
						Shader metallicRoughnessShader("gltf/shaders/pbr-metallicRoughness");
						Shader specularGlossinessShader("gltf/shaders/pbr-specularGlossiness");
						
						Shader * shaders[2] = { &metallicRoughnessShader, &specularGlossinessShader };
						for (auto * shader : shaders)
						{
							setShader(*shader);
							
						#if 1
							// directional light
							
							const float dx = cosf(framework.time / 3.45f);
							const float dz = sinf(framework.time / 4.56f);
							const Vec3 lightDir_world(dx, -2.f, dz);
							const Vec3 lightDir_view = camera.getViewMatrix().Mul3(lightDir_world).CalcNormalized();
					
							shader->setImmediate("scene_lightParams1",
								lightDir_view[0],
								lightDir_view[1],
								lightDir_view[2],
								0.f);
						#else
							// point light
							
							const float x = cosf(framework.time / 2.56f) * 3.f;
							const float y = 2.f + cosf(framework.time / 4.89f);
							const float z = sinf(framework.time / 3.67f) * 3.f;
							const Vec3 lightPos_world = Vec3(x, y, z) / scaleMultiplier;
							const Vec3 lightPos_view = viewMatrix.Mul4(lightPos_world);
					
							shader->setImmediate("scene_lightParams1",
								lightPos_view[0],
								lightPos_view[1],
								lightPos_view[2],
								1.f);
						#endif
						
							shader->setImmediate("scene_lightParams2",
								1.f,
								1.f,
								1.f,
								1.f);
						
							//shader->setImmediate("scene_ambientLightColor", .03f, .02f, .01f);
							shader->setImmediate("scene_ambientLightColor", .2f, .2f, .2f);
							
							clearShader();
							
						#if 0
							setColor(colorYellow);
							fillCube(lightPos_world, Vec3(.1f, .1f, .1f) * (centimeters ? 100.f : 1.f));
							
							/*
							logDebug("camera position: %.2f, %.2f, %.2f",
								camera.position[0],
								camera.position[1],
								camera.position[2]);
							*/
						#endif
						}
						
						gltf::MaterialShaders materialShaders;
						materialShaders.metallicRoughnessShader = &metallicRoughnessShader;
						materialShaders.specularGlossinessShader = &specularGlossinessShader;
						materialShaders.init();
						
						gltf::DrawOptions drawOptions;
						drawOptions.alphaMode =
							useAlphaToCoverage
							? gltf::kAlphaMode_AlphaToCoverage
							: gltf::kAlphaMode_AlphaBlend;
						
						gltf::drawScene(
							scene,
							keyboard.isDown(SDLK_m)
								? nullptr
								: bufferCache,
							materialShaders,
							isOpaquePass,
							&drawOptions);
					}
					popBlend();
					popDepthTest();
					popShaderOutputs();
				}
				
				if (keyboard.isDown(SDLK_b))
				{
					pushBlend(BLEND_ADD);
					pushDepthWrite(false);
					{
						for (size_t sceneRootIndex = 0; sceneRootIndex < scene.sceneRoots.size(); ++sceneRootIndex)
						{
							if (scene.activeScene != -1 && scene.activeScene != sceneRootIndex)
								continue;

							gltf::drawSceneMinMax(scene, sceneRootIndex);
						}
					}
					popDepthWrite();
					popBlend();
				}
			}
			camera.popViewMatrix();
			popBlend();
			popDepthTest();
			
			projectScreen2d();
			
			setColor(0, 0, 0, 127);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			hqFillRoundedRect(4, 4, VIEW_SX - 4, 90, 10.f);
			hqEnd();
			
			setFont("calibri.ttf");
			setColor(colorWhite);
			setLumi(170);
			
			drawText(10, 30, 16, +1, -1, "(Extra) Author: %s", scene.asset.extras.author.c_str());
			drawText(10, 50, 16, +1, -1, "(Extra) License: %s", scene.asset.extras.license.c_str());
			drawText(10, 70, 16, +1, -1, "(Extra) Title: %s", scene.asset.extras.title.c_str());
			
			drawText(10, VIEW_SY - 40, 16, +1, -1, "Meters to centimeter conversion ('C' to toggle): %s", centimeters ? "On" : "Off");
			drawText(10, VIEW_SY - 20, 16, +1, -1, "Alpha to coverage ('A' to toggle): %s", useAlphaToCoverage ? "On" : "Off");
		}
		framework.endDraw();
	}
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
