#include "forwardLighting.h"
#include "framework.h"
#include <vector>

/*

use the forward lighting helper to draw a scene using various area lights

*/

#if defined(DEBUG)
	#include "lightVolumeBuilder.h" // for drawing area light aabbs
#endif

using namespace rOne;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(800, 400))
		return -1;
	
	auto drawOpaque = [&]()
	{
		setColor(colorWhite);
		fillCylinder(Vec3(0, 1, 0), 1.f, 1.f, 4);
		
		fillCylinder(Vec3(-3, 1,  0), .2f, 1.f, 100);
		fillCylinder(Vec3(+3, 1,  0), .2f, 1.f, 100);
		fillCylinder(Vec3( 0, 1, -3), .2f, 1.f, 100);
		fillCylinder(Vec3( 0, 1, +3), .2f, 1.f, 100);
		
		gxPushMatrix();
		{
			gxTranslatef(0, 0, 0);
			gxScalef(10, 10, 10);
			setColor(200, 200, 200);
			drawGrid3d(1, 1, 0, 2);
		}
		gxPopMatrix();
	};
	
	struct AreaLight
	{
		LightType lightType;
		Mat4x4 transform = Mat4x4(true);
		float farDistance = 1.f;
		Vec3 color = Vec3(1, 1, 1);
		float intensity = 1.f;
	};
	
	ForwardLightingHelper helper;
	
	Camera3d camera;
	
	camera.position.Set(0, 6, -4);
	camera.pitch = -60.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		// -- add animated area lights --
		
		std::vector<AreaLight> areaLights;
		
		const Vec3 colors[3] =
		{
			Vec3(1, 1, 0),
			Vec3(0, 1, 1),
			Vec3(1, 0, 1)
		};

		const float time =
			keyboard.isDown(SDLK_t)
			? 6.f
			: fmodf(framework.time, 10.f);
		
		if (time <= 5.f)
		{
			const LightType lightTypes[4] =
			{
				kLightType_AreaBox,
				kLightType_AreaSphere,
				kLightType_AreaRect,
				kLightType_AreaCircle,
			};
			
			for (int i = 0; i < 4; ++i)
			{
				AreaLight areaLight;
				areaLight.lightType = lightTypes[i % 4];
				areaLight.transform = Mat4x4(true)
					.Translate(
						Vec3(
							sinf(i * 2.34f) * 4.f,
							sinf(framework.time * (1.23f + i * .6f)) * 2.f + 2.f,
							sinf(i * 3.45f) * 4.f + (i == 0 ? -2.5f : +2.5f)))
					.RotateY(framework.time)
					.RotateX(framework.time * 2.34f)
					.Scale(2.f, 1.f, .1f);
				areaLight.farDistance = 6.f;
				areaLight.color = colors[i % 3];
				areaLight.intensity = 1.f;
				
				areaLights.push_back(areaLight);
			}
		}
		else
		{
			const LightType lightTypes[4] =
			{
				kLightType_AreaBox,
				kLightType_AreaSphere,
				kLightType_AreaRect,
				kLightType_AreaCircle,
			};
			
			const Vec3 positions[4] =
			{
				Vec3(-4, 1, -4),
				Vec3(+4, 1, -4),
				Vec3(+4, 1, +4),
				Vec3(-4, 1, +4)
			};
			
			for (int i = 0; i < 4; ++i)
			{
				AreaLight areaLight;
				areaLight.lightType = lightTypes[i];
				areaLight.transform.MakeLookat(positions[i], positions[i] + Vec3(0, -1, 0), Vec3(1, 0, 0));
				areaLight.transform = areaLight.transform.CalcInv();
				areaLight.transform = Mat4x4(true).Translate(0, sinf(framework.time), 0).Mul(areaLight.transform).Scale(1.5f, 1, 1.f + sinf(framework.time / 2.34f));
				areaLight.farDistance = 6.f;
				areaLight.color = colors[i % 3];
				areaLight.intensity = 1.f;
				
				areaLights.push_back(areaLight);
			}
		}
		
		// -- determine view matrix --
		
		Mat4x4 worldToView = camera.getViewMatrix();
		
		if (keyboard.isDown(SDLK_1))
			worldToView = areaLights[0].transform.CalcInv();
		if (keyboard.isDown(SDLK_2))
			worldToView = areaLights[1].transform.CalcInv();
		if (keyboard.isDown(SDLK_3))
			worldToView = areaLights[2].transform.CalcInv();
		
		// -- prepare forward lighting --
		
		for (auto & areaLight : areaLights)
		{
			helper.addAreaLight(
				areaLight.lightType,
				areaLight.transform,
				0.f,
				areaLight.farDistance,
				areaLight.color,
				areaLight.intensity);
		}
		
		helper.addDirectionalLight(
			Vec3(1, 1, 1).CalcNormalized(),
			Vec3(1, 1, 1),
			.02f);
		
		helper.prepareShaderData(16, 32.f, true, worldToView);
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(90.f, .01f, 100.f);
			
			gxPushMatrix();
			gxLoadMatrixf(worldToView.m_v);
			{
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_OPAQUE);
				{
					Shader shader("210-light-with-shadow");
					setShader(shader);
					{
						int nextTextureUnit = 0;
						helper.setShaderData(shader, nextTextureUnit);
						
						drawOpaque();
					}
					clearShader();

					for (auto & areaLight : areaLights)
					{
						pushLineSmooth(true);
						gxPushMatrix();
						{
							gxMultMatrixf(areaLight.transform.m_v);
							setColor(255, 255, 255);
							lineCube(Vec3(), Vec3(1.f));
						}
						gxPopMatrix();
						popLineSmooth();
						
					#if defined(DEBUG)
						pushLineSmooth(true);
						gxPushMatrix();
						{
							Vec3 min;
							Vec3 max;
							LightVolumeBuilder::computeAreaLightAabb(areaLight.transform, areaLight.farDistance, min, max);
							
							setColor(127, 127, 255);
							lineCube((min + max) / 2.f, (max - min) / 2.f);
						}
						gxPopMatrix();
						popLineSmooth();
					#endif
						
						const char * lightTypeNames[4] =
						{
							"box",
							"sphere",
							"rect",
							"circle"
						};
						
						const int lightTypeIndex =
							areaLight.lightType == kLightType_AreaBox    ? 0 :
							areaLight.lightType == kLightType_AreaSphere ? 1 :
							areaLight.lightType == kLightType_AreaRect   ? 2 :
							areaLight.lightType == kLightType_AreaCircle ? 3 : -1;
						
						setColor(255, 127, 255);
						debugDrawText(
							areaLight.transform.GetTranslation()[0],
							areaLight.transform.GetTranslation()[1],
							areaLight.transform.GetTranslation()[2],
							12,
							0, 0,
							"%s",
							lightTypeNames[lightTypeIndex]);
					}
				}
				popBlend();
				popDepthTest();
			}
			gxPopMatrix();
			
			projectScreen2d();
			
		#if false
			setColor(colorWhite);
			drawText(4, 4, 12, +1, +1, "(%.2f, %.2f, %.2f)",
				camera.position[0],
				camera.position[1],
				camera.position[2]);
		#endif
		}
		framework.endDraw();
		
		helper.reset();
	}
	
	framework.shutdown();
	
	return 0;
}
