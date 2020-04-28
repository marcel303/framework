#include "framework.h"
#include "forwardLighting.h"
#include "gltf-material.h"

using namespace rOne;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 600))
		return -1;
	
	ForwardLightingHelper helper;
	
	Camera3d camera;
	camera.position[2] = -4.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
	#if 0
		helper.addSpotLight(
			Vec3(0, 8, 0),
			Vec3(0, -1, 0).CalcNormalized(),
			float(M_PI)*2/3,
			16.f,
			Vec3(1, 1, 1),
			100.f);
	#endif
	
		helper.addPointLight(
			Vec3(0, 8, 0),
			0.f,
			16.f,
			Vec3(1, 1, 1),
			100.f);
		
		framework.beginDraw(10, 10, 10, 0);
		{
			projectPerspective3d(70.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			
			camera.pushViewMatrix();
			{
				Mat4x4 worldToView;
				gxGetMatrixf(GX_MODELVIEW, worldToView.m_v);
				helper.prepareShaderData(16, 32.f, true, worldToView);
				
				//
				
				Shader shader("pbr");
				setShader(shader);
				{
					int nextTextureUnit = 0;
					helper.setShaderData(shader, nextTextureUnit);
					
					gltf::Material material;
					gltf::Scene scene;
				
					for (int x = -10; x <= +10; ++x)
					{
						for (int z = -10; z <= +10; ++z)
						{
							int nextTextureUnit2 = nextTextureUnit;
							material.pbrMetallicRoughness.metallicFactor = (x + 10) / 20.f;
							material.pbrMetallicRoughness.roughnessFactor = (z + 10) / 20.f;
							gltf::setShaderParams_metallicRoughness(shader, material, scene, true, nextTextureUnit2);
							
							gxPushMatrix();
							{
							#if 1
								Mat4x4 lookat;
								//lookat.MakeLookat(Vec3(x, 0, z), Vec3(0, 8, 0), Vec3(0, 1, 0));
								//lookat = lookat.CalcInv();
								lookat.MakeLookatInv(Vec3(x, 0, z), Vec3(0, 8, 0), Vec3(0, 1, 0));
								gxMultMatrixf(lookat.m_v);
								
								gxRotatef(framework.time * 40.f, (x/3)%2, 0, (z/3)%2);
							#else
								gxTranslatef(x, 0, z);
								gxRotatef(framework.time * 10.f, 1, 1, 1);
							#endif
								setColor(colorWhite);
								//fillCube(Vec3(), Vec3(.3f));
								fillCylinder(Vec3(), .15f, .45f, 20, 0.f, true);
							}
							gxPopMatrix();
						}
					}
				}
				clearShader();
				
				//fillCube(Vec3(), Vec3(1.f));
				
				//
				
				helper.reset();
			}
			camera.popViewMatrix();
			
			popDepthTest();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
