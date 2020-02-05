#include "framework.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(800, 600))
		return -1;
	
	Camera3d camera;
	camera.maxUpSpeed *= 2.f;
	camera.maxForwardSpeed *= 2.f;
	camera.maxStrafeSpeed *= 2.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			Shader shader("shader");
			setShader(shader);
			shader.setImmediateMatrix4x4("viewToWorld", camera.getWorldMatrix().m_v);
			drawRect(0, 0, 800, 600);
			clearShader();
			
			projectPerspective3d(90.f, .01f, 10.f);
			camera.pushViewMatrix();
			setColor(colorBlue);
			//drawGrid3d(10, 10, 0, 2);
			camera.popViewMatrix();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
