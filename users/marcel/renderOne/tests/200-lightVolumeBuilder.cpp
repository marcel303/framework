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
		
		auto data = builder.generateLightVolumeData();
		
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
	
	for (int i = 0; i < 1000; ++i)
	{
		lights.push_back(
			{
				Vec3(
					random<float>(-8.f, +8.f),
					random<float>(-.5f, +.5f),
					random<float>(-8.f, +8.f)),
				0.f,
				random<float>(.1f, .5f),
				Vec3(1, 1, 1) * random<float>(.1f, 1.f)
			});
	}
	
	ForwardLightingHelper helper;
	
	bool showLightVolumeOverlay = true;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		if (keyboard.wentDown(SDLK_v))
			showLightVolumeOverlay = !showLightVolumeOverlay;
		
		{
			for (size_t i = 0; i < lights.size(); ++i)
			{
				helper.addPointLight(
					lights[i].position,
					lights[i].att_begin,
					lights[i].att_end,
					lights[i].color,
					1.f);
			}
			
			const Mat4x4 worldToView = camera.getViewMatrix();
			
			helper.prepareShaderData(worldToView);
		}
	
		framework.beginDraw(0, 0, 0, 0);
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
				drawRect(0, 0, 800, 40);
				gxSetTexture(0);
		
				// show the light volume interpretation by the shader (2d)
				
				Shader shader("light-volume-2d");
				setShader(shader);
				{
					int nextTextureUnit = 0;
					helper.setShaderData(shader, nextTextureUnit);
					drawRect(100, 100, 200, 200);
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
						
						for (int i = 0; i < 1; ++i)
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
			drawText(4, 4, 12, +1, +1, "Press 'V' to toggle light volume overlay");
		}
		framework.endDraw();
		
		helper.reset();
	}
	
	framework.shutdown();
	
	return 0;
}
