#include "framework.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableRealTimeEditing = true;
	
	if (!framework.init(800, 800))
		return -1;

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;

		framework.beginDraw(0, 0, 0, 0);
		{
			Shader shader("gpupath");
			setShader(shader);
			shader.setImmediate("color1", 0.1, 0.3, 0.9, 1.0);
			shader.setImmediate("color2", 0.8, 1.0, 0.1, 1.0);
			shader.setImmediate("halfThickness", 5.f / 2.f);
			shader.setImmediate("hardness", 1.f);
			
			const GxImmediateIndex position1_idx = shader.getImmediateIndex("position1");
			const GxImmediateIndex position2_idx = shader.getImmediateIndex("position2");
			const GxImmediateIndex numVertices_idx = shader.getImmediateIndex("numVertices");
			
			for (int i = 0; i < 100; ++i)
			{
				const float x = mouse.x + cosf(framework.time * i / 100.f) * i * 2.f;
				const float y = mouse.y + sinf(framework.time * i / 100.f) * i * 2.f;
				
				const int numVertices = 200;
				
				shader.setImmediate(position1_idx, 10, 10);
				shader.setImmediate(position2_idx, x, y);
				shader.setImmediate(numVertices_idx, numVertices);
				{
					gxEmitVertices(GX_TRIANGLE_STRIP, numVertices);
				}
			}
			clearShader();
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
