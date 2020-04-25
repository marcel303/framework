#include "framework.h"

/*

area lights development stages:

1. create a plane and a light, using a basic shader to do all of the light calculations

1.1 implement a 2d and a 3d box (unit dimensions)
1.2 implement a sphere (unit dimensions)

2. implement the ability to scale and rotate the lights
	probable implementation: determine light matrix, invert it. let shader map points from world or view space into light space. perform calculations from before

3. optimization
	targets:
		- reduce shader constants
		- reduce shader computations

	attenuation distance needs to be done in world or view space (unscaled), but changing coordinate frame
	to light space means there is scaling involved
 
	can deduce scaling amount per xyz element by multiplying (1, 1, 1) with world to light matrix and seeing the change?
 
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
				const Mat4x4 viewToLight = lightToView.CalcInv();

				Shader shader("400-areaLight");
				setShader(shader);
				{
					shader.setImmediateMatrix4x4("viewToLight", viewToLight.m_v);
					shader.setImmediateMatrix4x4("lightToView", lightToView.m_v);
					
					{
						const float scaleX = lightToView.Mul3(Vec3(1, 0, 0)).CalcSize();
						const float scaleY = lightToView.Mul3(Vec3(0, 1, 0)).CalcSize();
						const float scaleZ = lightToView.Mul3(Vec3(0, 0, 1)).CalcSize();
						Mat4x4 rotationMatrix(true);
						for (int i = 0; i < 3; ++i)
						{
							rotationMatrix(0, i) = lightToView(0, i) / scaleX;
							rotationMatrix(1, i) = lightToView(1, i) / scaleY;
							rotationMatrix(2, i) = lightToView(2, i) / scaleZ;
						}
						logDebug("new scales: %.2f, %.2f, %.2f",
							rotationMatrix.Mul3(Vec3(1, 0, 0)).CalcSize(),
							rotationMatrix.Mul3(Vec3(0, 1, 0)).CalcSize(),
							rotationMatrix.Mul3(Vec3(0, 0, 1)).CalcSize());
						
						shader.setImmediateMatrix4x4("lightToView_rotation", rotationMatrix.m_v);
						shader.setImmediate("lightToView_translation",
							lightToView.GetTranslation()[0],
							lightToView.GetTranslation()[1],
							lightToView.GetTranslation()[2]);
						shader.setImmediate("lightToView_scale", scaleX, scaleY, scaleZ);
						
						Mat4x4 lightToView_packed = rotationMatrix;
						lightToView_packed.SetTranslation(lightToView.GetTranslation());
						lightToView_packed(0, 3) = scaleX;
						lightToView_packed(1, 3) = scaleY;
						lightToView_packed(2, 3) = scaleZ;
						lightToView_packed(3, 3) = areaLightType;
						shader.setImmediateMatrix4x4("lightToView_packed", lightToView_packed.m_v);
					}
					
					shader.setImmediate("areaLightType", (float)areaLightType);
					
					static int alternate = 0;
					shader.setImmediate("alternate", alternate);
					alternate = (alternate + 1) % 3;

					gxPushMatrix();
					gxScalef(4, 4, 4);
					drawGrid3d(1, 1, 0, 2);
					gxPopMatrix();
					
				#if false
					for (int i = 0; i < 100; ++i)
					{
						const float x = sinf(i) * 3.f;
						const float z = cosf(i) * 3.f;
						fillCube(Vec3(x, 0.f, z), Vec3(.1f));
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
			drawText(4, 4, 12, +1, +1, "Press [1..4] to add transformation(s)");
		}
		framework.endDraw();
	}

	framework.shutdown();
	
	return 0;
}
