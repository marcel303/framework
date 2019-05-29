#include "framework.h"

int main(int argc, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(800, 600))
		return -1;
	
	Camera3d camera;
	camera.yaw = -35;
	camera.pitch = -40;
	camera.position.Set(-2.25f, 2.58f, -2.80f);
	
	for (;;)
	{
		if (framework.quitRequested)
			break;
		
		framework.process();
		
		camera.tick(framework.timeStep, true);
		
		framework.beginDraw(0, 0, 0, 0);
		{
			Mat4x4 viewMatrix = camera.getViewMatrix();
		#if ENABLE_OPENGL
			viewMatrix = Mat4x4(true).Scale(1, -1, 1) * viewMatrix;
		#endif
		
			Shader shader("sd");
			setShader(shader);
			shader.setImmediate("u_resolution", 800.f, 600.f, 0, 0);
			shader.setImmediateMatrix4x4("u_invView", viewMatrix.CalcInv().m_v);
			drawRect(0, 0, 800, 600);
			clearShader();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
