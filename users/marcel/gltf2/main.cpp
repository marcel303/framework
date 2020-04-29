#include "framework.h"
#include "forwardLighting.h"
#include "shadowMapDrawer.h"
#include "gltf.h"
#include "gltf-material.h"

using namespace rOne;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 600))
		return -1;
	
	ForwardLightingHelper helper;
	
	ShadowMapDrawer shadowMapDrawer;
	shadowMapDrawer.alloc(4, 2048);
	
	Camera3d camera;
	camera.position[2] = -4.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		const Mat4x4 worldToView = camera.getViewMatrix();
		
		auto drawOpaqueBase = [&](const bool isShadowPass)
		{
			Shader shader("pbr");
			setShader(shader);
			{
				int nextTextureUnit = 0;
				shadowMapDrawer.setShaderData(shader, nextTextureUnit, worldToView);
				helper.setShaderData(shader, nextTextureUnit);
				
				gltf::Material material;
				gltf::Scene scene;
				
				gltf::MetallicRoughnessParams params;
				params.init(shader);
				params.setShaderParams(shader, material, scene, true, nextTextureUnit);
			
				gxPushMatrix();
				{
					params.setMetallicRoughness(shader, .05f, .2f);
					
					gxTranslatef(0, -2, 0);
					gxScalef(10, 10, 10);
					setColor(40, 40, 40);
					drawRect3d(0, 2);
				}
				gxPopMatrix();
				
				for (int x = -10; x <= +10; ++x)
				{
					for (int z = -10; z <= +10; ++z)
					{
						params.setMetallicRoughness(shader, (x + 10) / 20.f, (z + 10) / 20.f);
						
						gxPushMatrix();
						{
							Mat4x4 lookat;
							lookat.MakeLookatInv(Vec3(x, 0, z), Vec3(0, 8, 0), Vec3(0, 1, 0));
							gxMultMatrixf(lookat.m_v);
							gxRotatef(framework.time * 40.f, (x/3)%2, 0, (z/3)%2);

							setColor(colorWhite);
							fillCube(Vec3(), Vec3(.3f));
							//fillCylinder(Vec3(), .15f, .45f, 20, 0.f, true);
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
		spotTransform.MakeLookatInv(Vec3(-2, 8, 0), Vec3(0, 0, 0), Vec3(0, 1, 0));
		
		const float spotAngle = float(M_PI)*2/3;
		
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
			Vec3(1, 1, 1),
			50.f,
			shadowMapDrawer.getShadowMapId(0));
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
			projectPerspective3d(70.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			
			gxSetMatrixf(GX_MODELVIEW, worldToView.m_v);
			{
				helper.prepareShaderData(16, 32.f, true, worldToView);
				
				drawOpaque();
				
				helper.reset();
			}
			
			popDepthTest();
		}
		framework.endDraw();
		
		shadowMapDrawer.reset();
	}
	
	framework.shutdown();
	
	return 0;
}
