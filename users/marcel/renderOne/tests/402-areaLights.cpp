#include "framework.h"

/*

this is the cleaned up version of 400-areaLights, which is the test app used to implement the base functionality
it has various different ways to set shader constants. for this version, we will settle on the packed representation,
with only a single matrix used to go to/from light space, assuming an orthonormal matrix to begin with

*/

enum AreaLightType
{
	kAreaLightType_Box,
	kAreaLightType_Sphere,
	kAreaLightType_Rect,
	kAreaLightType_Circle
};

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 600))
		return -1;

	Camera3d camera;

	AreaLightType areaLightType = kAreaLightType_Box;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		camera.tick(framework.timeStep, true);
		
		if (keyboard.wentDown(SDLK_b))
			areaLightType = kAreaLightType_Box;
		if (keyboard.wentDown(SDLK_s))
			areaLightType = kAreaLightType_Sphere;
		if (keyboard.wentDown(SDLK_r))
			areaLightType = kAreaLightType_Rect;
		if (keyboard.wentDown(SDLK_c))
			areaLightType = kAreaLightType_Circle;

		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(70.f, .01f, 100.f);
			pushDepthTest(true, DEPTH_LESS);
			
			camera.pushViewMatrix();
			{
				const Mat4x4 lightToWorld =
					Mat4x4(true)
						.Translate(0, 2 + sinf(framework.time), 0)
						.Scale(1.f, 1.f + keyboard.isDown(SDLK_2) * sinf(framework.time * 10.f), 1.f)
						.Rotate(keyboard.isDown(SDLK_1) * framework.time * 20.f, Vec3(0, 1, 3))
						.Rotate(keyboard.isDown(SDLK_4) * framework.time * 1.f, Vec3(0, 1, 3))
						//.Rotate(framework.time, Vec3(1, 2, 3).CalcNormalized())
						.Rotate(float(M_PI/2.0), Vec3(1, 0, 0))
						.Scale(1.f, 1.f + keyboard.isDown(SDLK_3) * sinf(framework.time * 3.f) * .8f, 1.f)
						//.Scale(.1f, 1, 1);
						.Scale(1, 1, 1);
				
				const Mat4x4 lightToView = camera.getViewMatrix() * lightToWorld;

				Shader shader("402-areaLight");
				setShader(shader);
				{
					// create packed area light params
					Mat4x4 lightToView_packed(true);
					
					const float scaleX = lightToView.GetAxis(0).CalcSize();
					const float scaleY = lightToView.GetAxis(1).CalcSize();
					const float scaleZ = lightToView.GetAxis(2).CalcSize();
					
					// create orthonormal 3x3 rotation matrix
					for (int i = 0; i < 3; ++i)
					{
						lightToView_packed(0, i) = lightToView(0, i) / scaleX;
						lightToView_packed(1, i) = lightToView(1, i) / scaleY;
						lightToView_packed(2, i) = lightToView(2, i) / scaleZ;
					}
					
					// pack scaling factors
					lightToView_packed(0, 3) = scaleX;
					lightToView_packed(1, 3) = scaleY;
					lightToView_packed(2, 3) = scaleZ;
					
					// pack translation
					lightToView_packed.SetTranslation(lightToView.GetTranslation());
					
					// pack area light type
					lightToView_packed(3, 3) = areaLightType;
					
					// set shader constants
					shader.setImmediateMatrix4x4("lightToView_packed", lightToView_packed.m_v);
					shader.setImmediate("time", framework.time);

					//
					
					gxPushMatrix();
					gxScalef(4, 4, 4);
					drawGrid3d(1, 1, 0, 2);
					gxPopMatrix();
					
				#if false
					for (int i = 0; i < 100; ++i)
					{
						const float x = sinf(i) * 3.f;
						const float z = cosf(i) * 3.f;
						fillCube(Vec3(x, .1f, z), Vec3(.1f));
					}
				#endif
					
					gxPushMatrix();
					gxMultMatrixf(lightToWorld.m_v);
					pushLineSmooth(true);
					{
					#if 1
						lineCube(Vec3(), Vec3(1.f)); // 3d area lights (box and sphere area lights)
					#else
						drawRect(-1, -1, +1, +1); // 2d area lights (rect and circle area lights)
					#endif
					}
					popLineSmooth();
					gxPopMatrix();
				}
				clearShader();
			}
			camera.popViewMatrix();
			
			popDepthTest();
			
			projectScreen2d();
			setColor(255, 255, 255, 80);
			drawText(4, 4, 12, +1, +1, "Press [1..4] to add transformation(s)");fine
		}
		framework.endDraw();
	}

	framework.shutdown();
	
	return 0;
}
