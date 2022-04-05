#include "forwardLighting.h"
#include "lightVolumeBuilder.h"
#include "renderer.h"

#include "framework.h"

using namespace rOne;

struct LightParams
{
	char type;
	
	Vec3 position;

	float att_begin;
	float att_end;
	
	Vec3 color;
};

int main(int argc, char * argv[])
{
	{
		// light volume builder test. for stepping into with the debugger
		
		LightVolumeBuilder builder;
		
		builder.addPointLight(1, Vec3(-1, 0, -4), 3.f);
		builder.addPointLight(2, Vec3(+4, 0, -4), 4.f);
		builder.addPointLight(3, Vec3(+4, 0, +4), 2.f);
		builder.addPointLight(4, Vec3(-4, 0, +4), 1.f);
		
		builder.addSpotLight(5, Vec3(0, 0, 0), Vec3(1, 1, 1).CalcNormalized(), float(M_PI)/2.f, 1.f);
		
		auto data = builder.generateLightVolumeData(32, 16.f, false);
		
		data.free();
	}

	// visual test
	
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableRealTimeEditing = true;
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 800))
		return -1;
	
	Camera3d camera;
	
	std::vector<LightParams> lights =
	{
		{ 'p', Vec3(-1, 0, +4), 0.f, 3.f, Vec3(1, 1, 1) },
		{ 'p', Vec3(+4, 0, +4), 0.f, 4.f, Vec3(0, 1, 1) },
		{ 'p', Vec3(+4, 0, -4), 0.f, 2.f, Vec3(1, 0, 1) },
		{ 'p', Vec3(-4, 0, -4), 0.f, 1.f, Vec3(1, 1, 0) },
		{ 's', Vec3( 0, 1,  0), 0.f, 1.f, Vec3(1, 1, 1) },
	};
	
#if true
	for (int i = 0; i < 1000; ++i)
	{
		lights.push_back(
			{
				'p',
				Vec3(
					random<float>(-8.f, +8.f),
					random<float>(-1.f, +1.f),
					random<float>(-8.f, +8.f)),
				.1f,
				random<float>(.1f, .5f),
				Vec3(1, 1, 1) * random<float>(.1f, 1.f)
			});
	}
#endif
	
	ForwardLightingHelper helper;
	
	bool showLightVolumeOverlay = true;
	bool showLightsOutlines = true;
	bool animateLights = false;
	bool infiniteSpaceMode = false;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		if (keyboard.wentDown(SDLK_v))
			showLightVolumeOverlay = !showLightVolumeOverlay;
		
		if (keyboard.wentDown(SDLK_o))
			showLightsOutlines = !showLightsOutlines;
		
		if (keyboard.wentDown(SDLK_a))
			animateLights = !animateLights;
		
		if (keyboard.wentDown(SDLK_i))
			infiniteSpaceMode = !infiniteSpaceMode;
		
		{
			for (size_t i = 0; i < lights.size(); ++i)
			{
				if (lights[i].type == 'p')
				{
					const float scale =
						animateLights
						? (cosf(framework.time * 2.f + i) + 1.f) / 2.f
						: 1.f;
					
					helper.addPointLight(
						lights[i].position,
						lights[i].att_begin * scale,
						lights[i].att_end * scale,
						lights[i].color,
						1.f);
				}
				else if (lights[i].type == 's')
				{
					const float scale =
						animateLights
						? (cosf(framework.time * 2.f + i) + 1.f) / 2.f
						: 1.f;
					
					helper.addSpotLight(
						lights[i].position,
						Vec3(0, -1, 0),
						M_PI/2.f * scale,
						lights[i].att_end,
						Vec3(1, 1, 1),
						1.f);
				}
			}
			
			const Mat4x4 worldToView = camera.getViewMatrix();
			
			helper.prepareShaderData(16, 16.f, infiniteSpaceMode, worldToView);
		}
	
		framework.beginDraw(20, 20, 20, 0);
		{
			if (showLightVolumeOverlay)
			{
				// show light volume data
				
				Shader indicesShader("light-indices-2d");
				setShader(indicesShader);
				{
					indicesShader.setTexture3d("lightVolume", 0, helper.indexTexture.id, false, false);
					indicesShader.setImmediate("lightVolumeDims",
						helper.indexTexture.sx,
						helper.indexTexture.sy,
						helper.indexTexture.sz);
					drawRect(0, 0, 800, 800);
				}
				clearShader();
				
				setColorf(1, 1, 1, 1, 1.f / 4);
				gxSetTexture(helper.lightIdsTextureId, GX_SAMPLE_LINEAR, true);
				drawRect(0, 0, 200, 200);
				gxClearTexture();
		
				// show the light volume interpretation by the shader (2d)
				
				Shader shader("light-volume-2d");
				setShader(shader);
				{
					shader.setImmediate("useLightVolume", 1.f);
					int nextTextureUnit = 0;
					helper.setShaderData(shader, nextTextureUnit);
					drawRect(300, 100, 500, 300);
				}
				clearShader();
			}
			
			// show the light volume interpretation by the shader (3d)
			
			projectPerspective3d(90.f, .01f, 100.f);
			{
				camera.pushViewMatrix();
				
				// opaque pass
				
				pushDepthTest(true, DEPTH_LESS);
				pushBlend(BLEND_OPAQUE);
				{
					if (showLightsOutlines)
					{
						// draw bounding boxes for the lights, to give an indication where the lights are positioned
						
						//beginCubeBatch(); // fixme : add cube patch for line cubes ?
						{
							for (auto & light : lights)
							{
								if (light.type != 'p')
									continue;
								
								setColor(100, 100, 100);
								lineCube(
									light.position,
									Vec3(
										light.att_end,
										light.att_end,
										light.att_end));
							}
						}
						//endCubeBatch();
						
						for (auto & light : lights)
						{
							if (light.type != 's')
								continue;
							
							Vec3 min;
							Vec3 max;
							LightVolumeBuilder builder;
							builder.computeSpotLightAabb(light.position, Vec3(0, -1, 0), float(M_PI)/2.f, 2.f, min, max);
							
							Vec3 position = (min + max) / 2.f;
							Vec3 extents = (max - min) / 2.f;
							
							setColor(200, 200, 200);
							lineCube(position, extents);
						}
					}
					
					// draw some geometry, lit using information from the light volume
					
					Shader shader("light-volume");
					setShader(shader);
					{
						shader.setImmediate("useLightVolume", 1.f);
						int nextTextureUnit = 0;
						helper.setShaderData(shader, nextTextureUnit);
						
						gxPushMatrix();
						{
							gxTranslatef(0, 0, 4);
							gxRotatef(framework.time * 20.f, 1, 2, 3);
					
							setColor(colorWhite);
							fillCube(Vec3(0, 0, 0), Vec3(6.f, .5f, 2.f));
						}
						gxPopMatrix();
						
						gxPushMatrix();
						{
							gxTranslatef(0, 0, -4);
							gxRotatef(framework.time * 30.f, 0, 0, 1);
					
							setColor(colorWhite);
							fillCube(Vec3(0, 0, 0), Vec3(6.f, .5f, 1.f));
						}
						gxPopMatrix();
						
						gxPushMatrix();
						{
							gxTranslatef(0, 0, 0);
							gxRotatef(framework.time * 10.f, 0, 1, 0);
					
							setColor(colorWhite);
							fillCube(Vec3(0, 0, 0), Vec3(2.f, 2.f, .1f));
						}
						gxPopMatrix();
					}
					clearShader();
				}
				popBlend();
				popDepthTest();
				
				// translucent pass
				
				pushDepthTest(true, DEPTH_LESS, false);
				pushBlend(BLEND_ADD);
				{
					// draw some geometry, lit using information from the light volume
					
					Shader shader("light-volume");
					setShader(shader);
					{
						shader.setImmediate("useLightVolume", 1.f);
						int nextTextureUnit = 0;
						helper.setShaderData(shader, nextTextureUnit);
						
						for (int i = 0; i < 0; ++i)
						{
							gxPushMatrix();
							{
								gxTranslatef(0, 0, 0);
								gxRotatef(90.f + sinf(framework.time / 1.23f) * 10.f + 90.f * i / 10.f, 1, 0, 0);
						
								setColor(colorRed);
								drawRect(-10, -10, +10, +10);
							}
							gxPopMatrix();
						}
					}
					clearShader();
				}
				popBlend();
				popDepthTest();
				
				camera.popViewMatrix();
			}
			projectScreen2d();
			
			setColor(colorWhite);
			int y = 4;
			drawText(4, y += 16, 12, +1, +1, "Press 'V' to toggle light volume overlay (%s)", showLightVolumeOverlay ? "on" : "off");
			drawText(4, y += 16, 12, +1, +1, "Press 'O' to toggle light outlines (%s)", showLightsOutlines ? "on" : "off");
			drawText(4, y += 16, 12, +1, +1, "Press 'A' to toggle light animation (%s)", animateLights ? "on" : "off");
			drawText(4, y += 16, 12, +1, +1, "Press 'I' to toggle infinite space mode (%s)", infiniteSpaceMode ? "on" : "off");
		}
		framework.endDraw();
		
		helper.reset();
	}
	
	helper.free();
	
	framework.shutdown();
	
	return 0;
}
