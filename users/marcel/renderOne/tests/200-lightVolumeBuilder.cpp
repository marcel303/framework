#include "lightVolumeBuilder.h"
#include "renderer.h"

#include "framework.h"

struct LightParams
{
	Vec3 position;

	float att_begin;
	float att_end;
};

int main(int argc, char * argv[])
{
	// light volume builder test. for stepping into with the debugger
	
	{
		LightVolumeBuilder builder;
		
		builder.addPointLight(1, Vec3(-1, 0, -4), 3.f);
		builder.addPointLight(2, Vec3(+4, 0, -4), 4.f);
		builder.addPointLight(3, Vec3(+4, 0, +4), 2.f);
		builder.addPointLight(4, Vec3(-4, 0, +4), 1.f);
		
		auto data = builder.generateLightVolumeData();
		(void)data;
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
		{ Vec3(-1, 0, +4), 0.f, 3.f },
		{ Vec3(+4, 0, +4), 0.f, 4.f },
		{ Vec3(+4, 0, -4), 0.f, 2.f },
		{ Vec3(-4, 0, -4), 0.f, 1.f }
	};
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		// generate light volume data using the (current) set of lights
		
		LightVolumeBuilder builder;
		
		{
			const Mat4x4 worldToView = camera.getViewMatrix();
			
			for (size_t i = 0; i < lights.size(); ++i)
			{
				const Vec3 & position_world = lights[i].position;
				const Vec3 position_view = worldToView.Mul4(position_world);
				
				builder.addPointLight(
					i,
					position_view,
					lights[i].att_end);
			}
		}
		
		auto data = builder.generateLightVolumeData();
	
		// create textures from data
		
		GxTextureId index_texture = createTextureFromRG32F(
			data.index_table,
			data.index_table_sx,
			data.index_table_sy,
			false,
			false);

		GxTextureId light_ids_texture = createTextureFromR32F(
			data.light_ids,
			data.light_ids_sx,
			1,
			false,
			false);
		
		// dispose of the data
		
		data.free();
		
		// create a buffer to make accessible the lights to the shader
		
		ShaderBuffer lightsBuffer;
		
		{
			const Mat4x4 worldToView = camera.getViewMatrix();
			
			Vec4 * params = new Vec4[lights.size() * 2];
			
			for (size_t i = 0; i < lights.size(); ++i)
			{
				const Vec3 & position_world = lights[i].position;
				const Vec3 position_view = worldToView.Mul4(position_world);
				
				params[i * 2 + 0][0] = 0.f;
				params[i * 2 + 0][1] = position_view[0];
				params[i * 2 + 0][2] = position_view[1];
				params[i * 2 + 0][3] = position_view[2];
				
				params[i * 2 + 1][0] = lights[i].att_begin;
				params[i * 2 + 1][1] = lights[i].att_end;
				params[i * 2 + 1][2] = 0.f;
				params[i * 2 + 1][3] = 0.f;
			}
			
			lightsBuffer.setData(params, lights.size() * 2 * sizeof(Vec4));
			
			delete [] params;
			params = nullptr;
		}
	
		framework.beginDraw(0, 0, 0, 0);
		{
			// show light volume data
			
			setColorf(1.f / data.light_ids_sx, 1.f / 4, 1);
			gxSetTexture(index_texture);
			drawRect(0, 0, 800, 800);
			gxSetTexture(0);
			
			setColorf(1, 1, 1, 1, 1.f / 4);
			gxSetTexture(light_ids_texture);
			drawRect(0, 0, 800, 40);
			gxSetTexture(0);
			
			// show the light volume interpretation by the shader (2d)
			
			Shader shader("light-volume-2d");
			setShader(shader);
			{
				shader.setTexture("lightVolume", 0, index_texture, false, false);
				shader.setTexture("lightIds", 1, light_ids_texture, false, false);
				shader.setBuffer("lightParamsBuffer", lightsBuffer);
				drawRect(100, 100, 200, 200);
			}
			clearShader();
			
			// show the light volume interpretation by the shader (3d)
			
			projectPerspective3d(90.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			{
				camera.pushViewMatrix();
				
				// draw bounding boxes for the lights, to give an indication where the lights are positioned
				
				for (auto & light : lights)
				{
					setColor(colorBlue);
					lineCube(
						light.position,
						Vec3(
							light.att_end,
							light.att_end,
							light.att_end));
				}
				
				// draw some geometry, lit using information from the light volume
				
				Shader shader("light-volume");
				setShader(shader);
				{
					shader.setTexture("lightVolume", 0, index_texture, false, false);
					shader.setTexture("lightIds", 1, light_ids_texture, false, false);
					shader.setBuffer("lightParamsBuffer", lightsBuffer);
					
					gxPushMatrix();
					{
						gxTranslatef(0, 0, 4);
						gxRotatef(framework.time * 20.f, 1, 2, 3);
				
						setColor(colorWhite);
						fillCube(Vec3(0, 0, 0), Vec3(6.f, .5f, 1.f));
					}
					gxPopMatrix();
					
					gxPushMatrix();
					{
						gxTranslatef(0, 0, -4);
						gxRotatef(framework.time * 100.f, 0, 0, 1);
				
						setColor(colorWhite);
						fillCube(Vec3(0, 0, 0), Vec3(6.f, .5f, 1.f));
					}
					gxPopMatrix();
				}
				clearShader();
				
				camera.popViewMatrix();
			}
			popDepthTest();
			projectScreen2d();
		}
		framework.endDraw();
		
		freeTexture(index_texture);
		freeTexture(light_ids_texture);
	}
	
	framework.shutdown();
	
	return 0;
}
