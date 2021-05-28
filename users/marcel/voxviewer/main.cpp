// viewer for MagicaVoxel .vox files
// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox.txt
// https://github.com/ephtracy/voxel-model/blob/master/MagicaVoxel-file-format-vox-extension.txt
// https://ephtracy.github.io/

#include "framework.h"
#include "gx_mesh.h"

#include "magicavoxel-framework.h"

#include "forwardLighting.h"
#include "lightDrawer.h"
#include "renderer.h"
#include "renderOptions.h"
#include "shadowMapDrawer.h"

#include "Quat.h"

#include <algorithm>
#include <stdint.h>
#include <vector>

#define USE_OPTIMIZED_SHADOW_SHADER 1

#define ENABLE_IBL 1

using namespace rOne;

static void drawSky(Vec3Arg viewOrigin, Vec3Arg sunDirection, const bool equirectMode);

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	framework.filedrop = true;
	
	//framework.fullscreen = true;
	
	//if (!framework.init(400, 300))
	//if (!framework.init(600, 600))
	if (!framework.init(1200, 900))
		return 0;

	Renderer::registerShaderOutputs();
	
	MagicaWorld world;
	//readMagicaWorld("marcel-02.vox", world);
	readMagicaWorld("room.vox", world);

	Camera3d camera;
	//camera.mouseSmooth = .97f;
	//camera.maxForwardSpeed = 10.f;
	
	GxMesh drawMesh;
	GxVertexBuffer vb;
	GxIndexBuffer ib;
	
	gxCaptureMeshBegin(drawMesh, vb, ib);
	{
		drawMagicaWorld(world);
	}
	gxCaptureMeshEnd();
	
	RenderOptions renderOptions;
	//renderOptions.renderMode = kRenderMode_DeferredShaded;
	renderOptions.renderMode = kRenderMode_ForwardShaded;
	//renderOptions.renderMode = kRenderMode_Flat;
	renderOptions.linearColorSpace = true;
	renderOptions.backgroundColor.Set(1, .5f, .25f);
	
	//renderOptions.screenSpaceAmbientOcclusion.enabled = true;
	renderOptions.screenSpaceAmbientOcclusion.strength = 1.f;
	
	//renderOptions.fog.enabled = true;
	renderOptions.fog.thickness = .001f;
	
	//renderOptions.debugRenderTargets = true;
	
	//renderOptions.chromaticAberration.enabled = true;
	renderOptions.chromaticAberration.strength = 1.f;
	
	renderOptions.bloom.enabled = true;
	renderOptions.bloom.blurSize = 20.f;
	renderOptions.bloom.strength = .2f;
	
	//renderOptions.lightScatter.enabled = true;
	renderOptions.lightScatter.strength = 1.f;
	renderOptions.lightScatter.numSamples = 100; // todo : bump to a higher number. fix light scatter getting more or less bright with # samples
	
	renderOptions.depthSilhouette.enabled = true;
	renderOptions.depthSilhouette.color.Set(0, 0, 0, .5f);
	
	renderOptions.fxaa.enabled = true;
	
	renderOptions.colorGrading.enabled = true;
	
	Renderer renderer;
	
	bool showSolids = false;
	bool showTranslucents = false;
	bool skyEnabled = false;
	bool enableShadowMaps = true;
	
	ForwardLightingHelper helper;
	ShadowMapDrawer shadowMapDrawer;
	shadowMapDrawer.alloc(4, 1024);
	shadowMapDrawer.enableColorShadows = false;
	shadowMapDrawer.shadowMapFilter = kShadowMapFilter_PercentageCloser_3x3;
	
	Surface skyTarget;
	skyTarget.init(128, 64, SURFACE_RGBA16F, false, true);
	
	GxTexture3d lookupTexture;
	
	for (;;)
	{
	#if !defined(DEBUG)
		mouse.showCursor(false);
		mouse.setRelative(true);
	#endif
	
		framework.process();

		if (framework.quitRequested)
			break;
		
		for (auto & file : framework.droppedFiles)
		{
			world.free();
			
			readMagicaWorld(file.c_str(), world);
			
			gxCaptureMeshBegin(drawMesh, vb, ib);
			{
				drawMagicaWorld(world);
			}
			gxCaptureMeshEnd();
		}
		
		if (keyboard.wentDown(SDLK_s))
			showSolids = !showSolids;
		
		if (keyboard.wentDown(SDLK_t))
			showTranslucents = !showTranslucents;
		
		if (keyboard.wentDown(SDLK_k))
			skyEnabled = !skyEnabled;
		
		if (keyboard.wentDown(SDLK_m))
			enableShadowMaps = !enableShadowMaps;
		
		camera.tick(framework.timeStep, true);
		
		// prepare sky texture
		
		Quat sunRotation;
		sunRotation.fromAngleAxis(sinf(framework.time/10.f) * M_PI/2.f * 1.1f, Vec3(1, 0.3f, -0.3f).CalcNormalized());
		const Vec3 sunPosition = camera.position + sunRotation.toMatrix().Mul3(Vec3(0, 40, 0));
		//const Vec3 sunPosition = camera.position + Vec3(0, 20, 20);
		const Vec3 sunDirection = (camera.position - sunPosition).CalcNormalized();
		
		pushSurface(&skyTarget, true);
		{
			drawSky(Vec3(), sunDirection, true);
		}
		popSurface();
		
		skyTarget.gaussianBlur(10.f, 10.f);

	#if 1
		renderOptions.colorGrading.lookupTextureId = getTexture3d("color-grading-01.png").id;
	#elif 0
		renderOptions.colorGrading.lookupTextureFromSrgbColorTransform(
			[](Color & color)
			{
				color = color
					.hueShift(framework.time / 12.f)
					//.invertRGB(sinf(framework.time / 3.45f))
					.desaturate(sinf(framework.time / 2.34f));
			},
			lookupTexture);
		renderOptions.colorGrading.lookupTextureId = lookupTexture.id;
	#endif
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			
			camera.pushViewMatrix();
			{
			#if true
				{
					// update light scatter origin
					
					Mat4x4 projection;
					Mat4x4 modelView;
					gxGetMatrixf(GX_PROJECTION, projection.m_v);
					gxGetMatrixf(GX_MODELVIEW, modelView.m_v);
					
					const Vec4 sunPosition_world(sunPosition[0], sunPosition[1], sunPosition[2], 1.f);
					const Vec4 sunPosition_view = modelView * sunPosition_world;
					const Vec4 sunPosition_projection = projection * sunPosition_view;
					const Vec4 sunPosition_clip = sunPosition_projection / sunPosition_projection[3];
					const Vec2 sunPosition_tex((sunPosition_clip[0] + 1.f) / 2.f, (-sunPosition_clip[1] + 1.f) / 2.f);
					
					renderOptions.lightScatter.origin.Set(sunPosition_tex[0], sunPosition_tex[1]);
					
					// update light scatter strength. fade out when not facing the sun
					
					const Vec3 lightDirection_view =
						Vec3(
							sunPosition_view[0],
							sunPosition_view[1],
							sunPosition_view[2]).CalcNormalized();
					renderOptions.lightScatter.strengthMultiplier = fmaxf(0.f, lightDirection_view[2]);
				}
			#endif
				
				// -- make light list --
				
				std::vector<Light> lights;
				
				if (true)
				{
					Light light;
					light.type = kLightType_Directional;
					light.position = camera.position - sunDirection * 50.f;
					light.direction = sunDirection;
					light.attenuationBegin = 0.f;
					light.attenuationEnd = 100.f;
					light.color.Set(1, 1, 1);
					light.intensity = .01f;
					
					lights.push_back(light);
				}
				
				if (true)
				{
					// camera following point light
					
					Light light;
					light.type = kLightType_Point;
					light.position = camera.position;
					light.attenuationBegin = 0.f;
					light.attenuationEnd = 4.f;
					light.color.Set(1, 1, 1);
					light.intensity = .1f;
					
					lights.push_back(light);
				}
				
				if (true)
				{
					// rotating red spot light
					
					Light light;
					light.type = kLightType_Spot;
					light.position.Set(2, sinf(framework.time * 2.34f) * .4f, -2);
					//light.direction = Mat4x4(true).RotateY(framework.time * 4.f).GetAxis(2);
					light.direction = Mat4x4(true).RotateY(framework.time / 2.f).GetAxis(2);
					//light.direction = Mat4x4(true).RotateY(float(M_PI)).GetAxis(2);
					light.attenuationBegin = 0.01f;
					light.attenuationEnd = 6.f;
					light.color.Set(1, 0, 0);
					light.spotAngle = 70.f * float(M_PI/180.0);
					light.intensity = 4.f;
					
					lights.push_back(light);
				}
				
				if (true)
				{
					// pulsating white point light
					
					Light light;
					light.type = kLightType_Point;
					light.position.Set(0, 0, 0);
					light.attenuationBegin = 0.f;
					light.attenuationEnd = 2.f + sinf(framework.time * 2.34f) * 2.f;
					light.color.Set(1, 1, 1);
					light.intensity = 4.f;
					
					lights.push_back(light);
				}
				
				auto drawOpaqueBase = [&](const bool isMainPass)
				{
				#if USE_OPTIMIZED_SHADOW_SHADER
					Shader shader(
						isMainPass ?
							(
								renderOptions.renderMode == kRenderMode_ForwardShaded
								? "shader-forward"
								: "shader"
							)
						: "shader-shadow");
				#else
					Shader shader(
						renderOptions.renderMode == kRenderMode_ForwardShaded
						? "shader-forward"
						: "shader");
				#endif
					setShader(shader);
					
					int nextTextureUnit = 0;
					helper.setShaderData(shader, nextTextureUnit);
					shadowMapDrawer.setShaderData(shader, nextTextureUnit, camera.getViewMatrix());
				
				#if ENABLE_IBL
				// todo : make IBL part of the forward lighting helper, or add IBL helper
					shader.setTexture("ibl", nextTextureUnit++, skyTarget.getTexture(), true, false);
					shader.setImmediateMatrix4x4("viewToWorld", camera.getViewMatrix().CalcInv().m_v);
				#endif
					
					gxPushMatrix();
					{
						gxScalef(-1, 1, 1);
						pushCullFlip();
						
						gxRotatef(-90, 1, 0, 0);
						gxScalef(.1f, .1f, .1f);
						
						pushCullMode(CULL_BACK, CULL_CCW);
						{
						#if 1
							drawMesh.draw();
						#else
							drawMagicaWorld(world);
						#endif
						}
						popCullMode();
					
						popCullFlip();
					}
					gxPopMatrix();
					
					if (showSolids)
					{
						pushCullMode(CULL_BACK, CULL_CCW);
						beginCubeBatch();
						{
							for (int i = 0; i < 3*1000; ++i)
							{
								//const float s = .2f;
								const float s = lerp<float>(.04f, .14f, (cosf(framework.time + i) + 1.f) / 2.f);
								const float m = lerp<float>(-2.f, +2.f, (cosf(framework.time / 3.45f + i) + 1.f) / 2.f);
								Vec3 t;
								t[i % 3] = m;
								
								Assert(s >= 0.f);
								setColor(colorWhite);
								fillCube(
									Vec3(
										cosf(i / 1.23f) * 6.f,
										cosf(i / 2.34f) * 2.f,
										cosf(i * 1.23f) * 6.f) + t,
									Vec3(s, s, s));
							}
						}
						endCubeBatch();
						popCullMode();
					}
					
					clearShader();
				};
				
				auto drawOpaque = [&]()
				{
					drawOpaqueBase(true);
				};
				
				auto drawOpaqueShadow = [&]()
				{
					drawOpaqueBase(false);
				};
				
				auto drawBackground = [&]()
				{
					if (skyEnabled)
					{
					#if 1
						drawSky(camera.position, sunDirection, false);
					#else
						// draw the sky using the IBL texture we generated for it
						Shader shader("equirect-sky");
						setShader(shader);
						shader.setTexture("source", 0, skyTarget.getTexture(), true, false);
						shader.setImmediate("viewOrigin", camera.position[0], camera.position[1], camera.position[2]);
						fillCube(Vec3(), Vec3(40.f));
						clearShader();
					#endif
					
						//setColor(colorYellow);
						//fillCube(sunPosition, Vec3(.4f, .4f, .4f));
					}
				};
				
				auto drawTranslucent = [&]()
				{
					if (showTranslucents)
					{
						// draw some (semi-)transparent cubes
						
						Shader shader("translucent");
						setShader(shader);
						{
							int nextTextureUnit = 0;
							helper.setShaderData(shader, nextTextureUnit);
							shadowMapDrawer.setShaderData(shader, nextTextureUnit, camera.getViewMatrix());
							
						#if ENABLE_IBL
							shader.setTexture("ibl", nextTextureUnit++, skyTarget.getTexture(), true, false);
							shader.setImmediateMatrix4x4("viewToWorld", camera.getViewMatrix().CalcInv().m_v);
						#endif
						
							const int kNumCubes = 1000;
							Vec3 positions[kNumCubes];
							
							for (int i = 0; i < kNumCubes; ++i)
							{
								positions[i].Set(
									cosf(i / 1.23f) * 6.f,
									cosf(i / 2.34f) * 2.f,
									cosf(i * 1.23f) * 6.f);
								
								const float m = lerp<float>(-2.f, +2.f, (cosf(framework.time / 3.45f + i) + 1.f) / 2.f);
								positions[i][i % 3] += m;
							}
							
							Vec3 positions_view[kNumCubes];
							int indices[kNumCubes];
							Mat4x4 worldToView;
							gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
							for (int i = 0; i < kNumCubes; ++i)
							{
								positions_view[i] = worldToView.Mul4(positions[i]);
								indices[i] = i;
							}
							
							std::sort(indices, indices + kNumCubes, [&](int i1, int i2)
								{
									const float z1 = positions_view[i1][2];
									const float z2 = positions_view[i2][2];
									return z2 < z1;
								});
							
							pushCullMode(CULL_BACK, CULL_CCW);
							beginCubeBatch();
							{
								setColorf(1, 1, 1, .5f);
								for (int i = 0; i < kNumCubes; ++i)
									fillCube(positions[indices[i]], Vec3(.2f, .2f, .2f));
							}
							endCubeBatch();
							popCullMode();
						}
						clearShader();
					}
				};
				
				auto drawLights = [&]()
				{
					//g_lightDrawer.drawDeferredAmbientLight(Vec3(1, 1, 1), .01f);
					
					for (auto & light : lights)
					{
						if (light.type == kLightType_Directional)
						{
							g_lightDrawer.drawDeferredDirectionalLight(
								light.direction,
								light.color,
								light.color,
								light.intensity);
						}
						else if (light.type == kLightType_Point)
						{
							g_lightDrawer.drawDeferredPointLight(
								light.position,
								light.attenuationBegin,
								light.attenuationEnd,
								light.color,
								light.intensity);
						}
						else if (light.type == kLightType_Spot)
						{
							g_lightDrawer.drawDeferredSpotLight(
								light.position,
								light.direction,
								light.spotAngle,
								light.attenuationBegin,
								light.attenuationEnd,
								light.color,
								light.intensity);
						}
					}
				};
				
				// -- prepare shadow maps --
				
				size_t lightId = 0;
				
				if (enableShadowMaps)
				{
					for (auto & light : lights)
					{
						if (light.type == kLightType_Directional)
						{
							Mat4x4 lightToWorld;
							lightToWorld.MakeLookat(light.position, light.position + light.direction, Vec3(0, 1, 0));
							lightToWorld = lightToWorld.CalcInv();
						
							shadowMapDrawer.addDirectionalLight(
								lightId,
								lightToWorld,
								light.attenuationBegin,
								light.attenuationEnd,
								8.f);
						}
						else if (light.type == kLightType_Spot)
						{
							Mat4x4 lightToWorld;
							lightToWorld.MakeLookat(light.position, light.position + light.direction, Vec3(0, 1, 0));
							lightToWorld = lightToWorld.CalcInv();
						
							shadowMapDrawer.addSpotLight(
								lightId,
								lightToWorld,
								light.spotAngle,
								light.attenuationBegin,
								light.attenuationEnd);
						}
						
						lightId++;
					}
				}
				
				helper.prepareShaderData(4, 64.f, false, camera.getViewMatrix());
				
				shadowMapDrawer.drawOpaque = drawOpaqueShadow;
				shadowMapDrawer.drawTranslucent = drawTranslucent;
				shadowMapDrawer.drawShadowMaps(camera.getViewMatrix());
				
				helper.reset();
				
				// -- prepare lighting data --
				
				lightId = 0;
				
				for (auto & light : lights)
				{
					if (light.type == kLightType_Directional)
					{
						helper.addDirectionalLight(
							light.direction,
							light.color,
							light.intensity,
							shadowMapDrawer.getShadowMapId(lightId));
					}
					else if (light.type == kLightType_Point)
					{
						helper.addPointLight(
							light.position,
							light.attenuationBegin,
							light.attenuationEnd,
							light.color,
							light.intensity,
							shadowMapDrawer.getShadowMapId(lightId));
					}
					else if (light.type == kLightType_Spot)
					{
						helper.addSpotLight(
							light.position,
							light.direction,
							light.spotAngle,
							light.attenuationEnd,
							light.color,
							light.intensity,
							shadowMapDrawer.getShadowMapId(lightId));
					}
				
					lightId++;
				}
				
				helper.prepareShaderData(32, 64.f, true, camera.getViewMatrix());
				
				// -- render --
				
				RenderFunctions renderFunctions;
				renderFunctions.drawOpaque = drawOpaque;
				renderFunctions.drawBackground = drawBackground;
				renderFunctions.drawTranslucent = drawTranslucent;
				renderFunctions.drawLights = drawLights;
			
				renderer.render(renderFunctions, renderOptions, framework.timeStep);
			}
			camera.popViewMatrix();
			
			projectScreen2d();
			
			renderOptions.debugRenderTargets = keyboard.isDown(SDLK_1);
				
			if (keyboard.isDown(SDLK_2))
				shadowMapDrawer.showRenderTargets();
			
			if (keyboard.isDown(SDLK_3))
			{
				gxSetTexture(skyTarget.getTexture());
				setColor(colorWhite);
				drawRect(0, 0, skyTarget.getWidth(), skyTarget.getHeight());
				gxSetTexture(0);
			}
		}
		framework.endDraw();
		
		helper.reset();
		shadowMapDrawer.reset();
	}
	
	lookupTexture.free();
	
	skyTarget.free();
	
	shadowMapDrawer.free();
	helper.free();
	
	renderer.free();
	
	drawMesh.clear();
	ib.free();
	vb.free();
	
	world.free();
	
	framework.shutdown();

	return 0;
}

// todo : create dedicated functions for drawing IBL texture

static void drawSky(Vec3Arg viewOrigin, Vec3Arg sunDirection, const bool equirectMode)
{
	pushBlend(BLEND_OPAQUE);
	pushDepthWrite(false);
	{
		const Vec3 lightPosition = viewOrigin;
		const Vec3 lightDirection = sunDirection;
		
		Shader shader("rui-sky");
		setShader(shader);
		{
			shader.setImmediate("lightPosition",
				lightPosition[0],
				lightPosition[1],
				lightPosition[2]);
			
			shader.setImmediate("lightDirection",
				lightDirection[0],
				lightDirection[1],
				lightDirection[2]);
				
			shader.setImmediate("uvToEquirectMode", equirectMode ? 1.f : 0.f);
			
			if (equirectMode)
			{
				gxMatrixMode(GX_PROJECTION);
				gxPushMatrix();
				gxLoadIdentity();
				gxMatrixMode(GX_MODELVIEW);
				gxPushMatrix();
				gxLoadIdentity();
				
				drawRect(-1, -1, +1, +1);
				
				gxMatrixMode(GX_PROJECTION);
				gxPopMatrix();
				gxMatrixMode(GX_MODELVIEW);
				gxPopMatrix();
			}
			else
			{
				fillCube(Vec3(), Vec3(40, 40, 40));
			}
		}
		clearShader();
	}
	popDepthWrite();
	popBlend();
}
