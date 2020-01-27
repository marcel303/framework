#include "framework.h"
#include "gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"
#include "Quat.h"

#define VIEW_SX 1000
#define VIEW_SY 600

#define USE_BUFFER_CACHE 1 // create static vertex and index buffers from gltf resources

#define ANIMATED_CAMERA 0 // todo : remove option and use hybrid

#define LOW_LATENCY_HACK_TEST 0 // todo : remove

#if LOW_LATENCY_HACK_TEST
	#include <unistd.h>
#endif

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

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	framework.msaaLevel = 4;
	framework.filedrop = true;
	
	if (!framework.init(VIEW_SX, VIEW_SY))
		return -1;

	//const char * path = "van_gogh_room/scene.gltf";
	//const char * path = "littlest_tokyo/scene.gltf";
	//const char * path = "ftm/scene.gltf";
	const char * path = "nara_the_desert_dancer/scene.gltf";
	//const char * path = "halloween_little_witch/scene.gltf";
	//const char * path = "kalestra_the_sorceress/scene.gltf";

	gltf::Scene scene;
	
	if (!gltf::loadScene(path, scene))
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
	
	bool centimeters = false;
	
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
				bufferCache = new gltf::BufferCache();
				bufferCache->init(scene);
			}
		}
		
		if (keyboard.wentDown(SDLK_t))
			centimeters = !centimeters;
		
	#if ANIMATED_CAMERA
		if (keyboard.wentDown(SDLK_p))
		{
			gltf::BoundingBox boundingBox;
			
			calculateSceneMinMaxTraverse(scene, scene.activeScene, boundingBox);
			
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
				if (centimeters)
					gxScalef(scaleMultiplier, scaleMultiplier, scaleMultiplier);
				gxScalef(1, 1, -1);
				
				for (int i = 0; i < 2; ++i)
				{
					const bool isOpaquePass = (i == 0);
					
					pushDepthWrite(!keyboard.isDown(SDLK_z) ? true : (isOpaquePass ? true : false));
					pushBlend(isOpaquePass ? BLEND_OPAQUE : BLEND_ALPHA);
					{
						Shader metallicRoughnessShader("gltf/shaders/shader-pbr-metallicRoughness");
						Shader specularGlossinessShader("gltf/shaders/shader-pbr-specularGlossiness");
						
						Shader * shaders[2] = { &metallicRoughnessShader, &specularGlossinessShader };
						for (auto * shader : shaders)
						{
							setShader(*shader);
							shader->setImmediate("scene_camPos",
								0.f,
								0.f,
								0.f);
							
							const float dx = cosf(framework.time / 1.56f);
							const float dz = sinf(framework.time / 1.67f);
							const Vec3 lightDir_world(dx, 0.f, dz);
							const Vec3 lightDir_view = camera.getViewMatrix().Mul3(lightDir_world);
					
							shader->setImmediate("scene_lightDir",
								lightDir_view[0],
								lightDir_view[1],
								lightDir_view[2]);
							
							clearShader();
						}
						
						gltf::MaterialShaders materialShaders;
						materialShaders.pbr_specularGlossiness = &specularGlossinessShader;
						materialShaders.pbr_metallicRoughness = &metallicRoughnessShader;
						materialShaders.fallbackShader = &metallicRoughnessShader;
						
						gltf::drawScene(
							scene,
							keyboard.isDown(SDLK_m)
								? nullptr
								: bufferCache,
							materialShaders,
							isOpaquePass);
					}
					popBlend();
					popDepthWrite();
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
							
							auto & sceneRoot = scene.sceneRoots[sceneRootIndex];
							
							for (auto & node_index : sceneRoot.nodes)
							{
								if (node_index >= 0 && node_index < scene.nodes.size())
								{
									auto & node = scene.nodes[node_index];
									
									drawNodeMinMaxTraverse(scene, node);
								}
							}
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
		}
		framework.endDraw();
		
	#if LOW_LATENCY_HACK_TEST
		static uint64_t t1 = 0;
		static uint64_t t2 = 0;
		
		static int x = 0;
		x++;
		printf("frame: %d\n", x);
		
		t2 = SDL_GetTicks();
		
		const uint64_t time_ms = t2 - t1 + 1;
		
		if (time_ms < 16 && !keyboard.isDown(SDLK_RSHIFT))
		{
			const uint64_t delay_ms = 16 - time_ms;
			
			usleep(delay_ms * 1000);
		}
		
		t1 = SDL_GetTicks();
	#endif
	}
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();

	return 0;
}
