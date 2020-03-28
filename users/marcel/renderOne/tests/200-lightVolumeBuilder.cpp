#include "forwardLighting.h"
#include "lightVolumeBuilder.h"
#include "renderer.h"

#include "framework.h"

struct LightParams
{
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
		
		auto data = builder.generateLightVolumeData(32, 16.f);
		
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
		{ Vec3(-1, 0, +4), 0.f, 3.f, Vec3(1, 1, 1) },
		{ Vec3(+4, 0, +4), 0.f, 4.f, Vec3(0, 1, 1) },
		{ Vec3(+4, 0, -4), 0.f, 2.f, Vec3(1, 0, 1) },
		{ Vec3(-4, 0, -4), 0.f, 1.f, Vec3(1, 1, 0) }
	};
	
#if true
	for (int i = 0; i < 4000; ++i)
	{
		lights.push_back(
			{
				Vec3(
					random<float>(-8.f, +8.f),
					random<float>(-2.f, +2.f),
					random<float>(-8.f, +8.f)),
				0.f,
				random<float>(.1f, .5f),
				Vec3(1, 1, 1) * random<float>(.1f, 1.f)
			});
	}
#endif
	
	ForwardLightingHelper helper;
	
	bool showLightVolumeOverlay = true;
	bool showLightsOutlines = true;
	bool animateLights = true;
	
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
		
		{
			for (size_t i = 0; i < lights.size(); ++i)
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
			
			const Mat4x4 worldToView = camera.getViewMatrix();
			
			helper.prepareShaderData(32, 32.f, worldToView);
		}
	
		framework.beginDraw(20, 20, 20, 0);
		{
			if (showLightVolumeOverlay)
			{
				// show light volume data
				
				int indexTextureSx;
				int indexTextureSy;
				gxGetTextureSize(helper.indexTextureId, indexTextureSx, indexTextureSy);
				
				setColorf(1.f / indexTextureSx, 1.f / 4, 1);
				gxSetTexture(helper.indexTextureId);
				drawRect(0, 0, 800, 800);
				gxSetTexture(0);
				
				setColorf(1, 1, 1, 1, 1.f / 4);
				gxSetTexture(helper.lightIdsTextureId);
				drawRect(0, 0, 200, 200);
				gxSetTexture(0);
		
				// show the light volume interpretation by the shader (2d)
				
				Shader shader("light-volume-2d");
				setShader(shader);
				{
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
						
						beginCubeBatch();
						{
							for (auto & light : lights)
							{
								setColor(100, 100, 100);
								lineCube(
									light.position,
									Vec3(
										light.att_end,
										light.att_end,
										light.att_end));
							}
						}
						endCubeBatch();
					}
					
					// draw some geometry, lit using information from the light volume
					
					Shader shader("light-volume");
					setShader(shader);
					{
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
						int nextTextureUnit = 0;
						helper.setShaderData(shader, nextTextureUnit);
						
						for (int i = 0; i < 0; ++i)
						{
							gxPushMatrix();
							{
								gxTranslatef(0, 0, 0);
								gxRotatef(90.f + sinf(framework.time / 1.23f) * 10.f + 90.f * i / 10.f, 1, 0, 0);
						
								setColor(255, 255, 255);
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
			drawText(4, y += 16, 12, +1, +1, "Press 'V' to toggle light volume overlay");
			drawText(4, y += 16, 12, +1, +1, "Press 'O' to toggle light outlines");
			drawText(4, y += 16, 12, +1, +1, "Press 'A' to toggle light animation");
		}
		framework.endDraw();
		
		helper.reset();
	}
	
	framework.shutdown();
	
	return 0;
}
