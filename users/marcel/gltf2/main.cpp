#include "framework.h"

#include "renderer.h"

#include "forwardLighting.h"
#include "shadowMapDrawer.h"

#include "gltf.h"
#include "gltf-draw.h"
#include "gltf-loader.h"
#include "gltf-material.h"

/*

desired ways to draw GLTF scenes,
- managed material setup: with GLTF drawing managing shader setting, material setup
- skip material setup: with GLTF drawing not touching the active shader or material setup

managed material setup:
- provide GLTF drawing routines with which shaders to use
- needs first texture unit, so the shader can have other textures set on it, in addition to the ones set by the material setup
	- this should be per-material type (so x2 for now)

skip material setup:
- maximum performance. set shader only once. set minimum of per-draw call shader params before calling drawScene
	note : may still want to let GLTF draw switch shaders, depending on the material

default material: should be part of the drawing options
it needs to be controlled so that double-sidedness and blend mode can be controlled

*/

using namespace rOne;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	//framework.allowHighDpi = true;
	//framework.msaaLevel = 4;
	
	//framework.fullscreen = true;
	if (!framework.init(800, 600))
	//if (!framework.init(1000, 800))
		return -1;
	
	ForwardLightingHelper helper;
	
	ShadowMapDrawer shadowMapDrawer;
	shadowMapDrawer.alloc(4, 2048);
	
	Camera3d camera;
	camera.mouseSmooth = .98f;
	camera.position[2] = -4.f;
	
	gltf::Scene scene;
	gltf::loadScene("smooth-cube.gltf", scene);
	//gltf::loadScene("space-ship.gltf", scene);
	
	gltf::BufferCache bufferCache;
	bufferCache.init(scene);
	
	Renderer renderer;
	renderer.registerShaderOutputs();
	RenderOptions renderOptions;
	renderOptions.renderMode = kRenderMode_ForwardShaded;
	renderOptions.linearColorSpace = true;
	renderOptions.bloom.enabled = true;
	renderOptions.bloom.strength = .2f;
	renderOptions.depthSilhouette.enabled = true;
	renderOptions.depthSilhouette.color[3] = .01f;
	//renderOptions.colorGrading.enabled = true;
	//renderOptions.chromaticAberration.enabled = true;
	renderOptions.motionBlur.enabled = true;
	//renderOptions.lightScatter.enabled = true;
	//renderOptions.enableScreenSpaceReflections = true;
	
	mouse.setRelative(true);
	mouse.showCursor(false);
	
	float emissive = 0.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		emissive *= powf(.1f, framework.timeStep);
		
		if (keyboard.wentDown(SDLK_SPACE))
			emissive = .2f;
		
		const Mat4x4 worldToView = camera.getViewMatrix();
		
		auto drawOpaqueBase = [&](const bool isShadowPass)
		{
			Shader shader("pbr");
			setShader(shader);
			{
				int nextTextureUnit = 0;
				shadowMapDrawer.setShaderData(shader, nextTextureUnit, worldToView);
				helper.setShaderData(shader, nextTextureUnit);
				
				gltf::MetallicRoughnessParams params;
				params.init(shader);
				
				gltf::Material material;
				params.setShaderParams(shader, material, scene, false, nextTextureUnit);
			
				gxPushMatrix();
				{
					params.setUseVertexColors(shader, true);
					params.setMetallicRoughness(shader, .05f, .2f);
					
					gxTranslatef(0, -2, 0);
					gxScalef(10, 10, 10);
					setColor(10, 10, 10);
					drawRect3d(0, 2);
					
					params.setUseVertexColors(shader, false);
				}
				gxPopMatrix();
			
				gltf::MaterialShaders materialShaders;
				materialShaders.init();
				materialShaders.firstTextureUnit = nextTextureUnit;
				
				gltf::DrawOptions drawOptions;
				drawOptions.enableMaterialSetup = false;
				drawOptions.enableShaderSetting = false;
				drawOptions.activeScene = -1;
				
				params.setMetallicRoughness(shader, 1.f, (1.f + sinf(framework.time * 2.f)) / 2.f);
				params.setEmissive(shader, emissive);
				gltf::drawScene(scene, &bufferCache, materialShaders, true, &drawOptions);
				
				for (int x = -10; x <= +10; ++x)
				{
					for (int z = -10; z <= +10; ++z)
					{
						params.setBaseColor(shader, Color::fromHSL(x / 20.f, lerp<float>(0.f, .4f, z / 20.f + .5f), .5f));
						params.setMetallicRoughness(shader, (x + 10) / 20.f, (z + 10) / 20.f);
						
						gxPushMatrix();
						{
							Mat4x4 lookat;
							lookat.MakeLookatInv(Vec3(x, 0, z), Vec3(0, 8, 0), Vec3(0, 1, 0));
							gxMultMatrixf(lookat.m_v);
							gxRotatef(framework.time * 140.f, (x/3)%2, 0, (z/3)%2);

							setColor(colorWhite);
							//fillCube(Vec3(), Vec3(.3f));
							//fillCylinder(Vec3(), .12f, .4f, 20, 0.f, true);
							
							gxScalef(.4f, .4f, .4f);
							//gxScalef(.1f, .1f, .1f);
							gltf::drawScene(scene, &bufferCache, materialShaders, true, &drawOptions);
						}
						gxPopMatrix();
					}
				}
			}
			clearShader();
		
			//fillCube(Vec3(), Vec3(1.f));
		};
		
		auto drawOpaque = [&]()
		{
			drawOpaqueBase(false);
		};
		
		auto drawShadow = [&]()
		{
			drawOpaqueBase(true);
		};
		
	#if 1
		Mat4x4 spotTransform;
		spotTransform = Mat4x4(true)
			.Lookat(Vec3(-2, 8, 0), Vec3(0, 0, 0), Vec3(0, 1, 0))
			.RotateX(sinf(framework.time / 6.23f) * .5f)
			.RotateY(sinf(framework.time / 8.34f) * .5f);
		
		const float spotAngle = float(M_PI) * lerp<float>(.3f, .5f, (cosf(framework.time / 2.34f) + 1.f) / 2.f);
		const float intensityMultiplier = powf(1.f / tanf(spotAngle/2.f), 2.f);
		
		shadowMapDrawer.addSpotLight(0, spotTransform, spotAngle, .01f, 100.f);
		
		helper.prepareShaderData(1, 1.f, false, worldToView);
		shadowMapDrawer.drawOpaque = drawShadow;
		shadowMapDrawer.drawShadowMaps(worldToView);
		helper.reset();
		
		helper.addSpotLight(
			spotTransform.GetTranslation(),
			spotTransform.GetAxis(2),
			spotAngle,
			16.f,
			Vec3(.6f, .8f, 1.f),
			25.f * intensityMultiplier,
			shadowMapDrawer.getShadowMapId(0));
		
		helper.addDirectionalLight(Vec3(4, -4, 4).CalcNormalized(), Vec3(1.f, .8f, .6f), .1f);
	#endif
	
	#if 0
		helper.addPointLight(
			Vec3(0, 8, 0),
			0.f,
			16.f,
			Vec3(1, 1, 1),
			25.f);
	#endif
		
		framework.beginDraw(10, 10, 10, 0);
		{
			static float fov = 70.f;
			const float desiredFov = mouse.isDown(BUTTON_LEFT) ? 14.f : 60.f;
			fov = lerp<float>(desiredFov, fov, powf(.02f, framework.timeStep));
			
			projectPerspective3d(fov, .01f, 100.f);
			gxSetMatrixf(GX_MODELVIEW, worldToView.m_v);
			
			helper.prepareShaderData(16, 32.f, true, worldToView);
			
			//
		
		#if 1
			RenderFunctions renderFunctions;
			renderFunctions.drawOpaque = drawOpaque;
			
			renderer.render(renderFunctions, renderOptions, framework.timeStep);
		#else
			pushDepthTest(true, DEPTH_LESS);
			pushBlend(BLEND_OPAQUE);
			{
				drawOpaque();
			}
			popBlend();
			popDepthTest();
		#endif
			
			//
			
			helper.reset();
		}
		framework.endDraw();
		
		shadowMapDrawer.reset();
	}
	
	framework.shutdown();
	
	return 0;
}
