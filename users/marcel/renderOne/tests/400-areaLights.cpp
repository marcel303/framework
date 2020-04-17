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

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(800, 600))
		return -1;

	Camera3d camera;

	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		camera.tick(framework.timeStep, true);

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
						.Rotate(framework.time, Vec3(1, 2, 3).CalcNormalized())
						.Scale(.1f, 1, 1);
				const Mat4x4 worldToLight = lightToWorld.CalcInv();

				Shader shader("400-areaLight");
				setShader(shader);
				{
					shader.setImmediateMatrix4x4("viewToWorld", camera.getWorldMatrix().m_v);
					shader.setImmediateMatrix4x4("worldToLight", worldToLight.m_v);
					shader.setImmediateMatrix4x4("lightToWorld", lightToWorld.m_v);
					
					{
						const float scaleX = lightToWorld.Mul3(Vec3(1, 0, 0)).CalcSize();
						const float scaleY = lightToWorld.Mul3(Vec3(0, 1, 0)).CalcSize();
						const float scaleZ = lightToWorld.Mul3(Vec3(0, 0, 1)).CalcSize();
						Mat4x4 rotationMatrix(true);
						for (int i = 0; i < 3; ++i)
						{
							rotationMatrix(0, i) = lightToWorld(0, i) / scaleX;
							rotationMatrix(1, i) = lightToWorld(1, i) / scaleY;
							rotationMatrix(2, i) = lightToWorld(2, i) / scaleZ;
						}
						logDebug("new scales: %.2f, %.2f, %.2f",
							rotationMatrix.Mul3(Vec3(1, 0, 0)).CalcSize(),
							rotationMatrix.Mul3(Vec3(0, 1, 0)).CalcSize(),
							rotationMatrix.Mul3(Vec3(0, 0, 1)).CalcSize());
						
						shader.setImmediateMatrix4x4("lightToWorld_rotation", rotationMatrix.m_v);
						shader.setImmediate("lightToWorld_translation",
							lightToWorld.GetTranslation()[0],
							lightToWorld.GetTranslation()[1],
							lightToWorld.GetTranslation()[2]);
						shader.setImmediate("lightToWorld_scale", scaleX, scaleY, scaleZ);
						
						Mat4x4 lightToWorld_packed = rotationMatrix;
						lightToWorld_packed.SetTranslation(lightToWorld.GetTranslation());
						lightToWorld_packed(0, 3) = scaleX;
						lightToWorld_packed(1, 3) = scaleY;
						lightToWorld_packed(2, 3) = scaleZ;
						shader.setImmediateMatrix4x4("lightToWorld_packed", lightToWorld_packed.m_v);
					}
					
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
					lineCube(Vec3(), Vec3(1.f));
					popLineSmooth();
					gxPopMatrix();
				}
				clearShader();
			}
			camera.popViewMatrix();
			
			popDepthTest();
		}
		framework.endDraw();
	}

	framework.shutdown();
	return 0;
}
