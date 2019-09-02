#include "framework.h"

int main(int argc, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
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
			shader.setImmediate("numVertices", 1000);
			shader.setImmediate("halfThickness", 5.f / 2.f);
			shader.setImmediate("hardness", .5f);
			
			const GxImmediateIndex position1_idx = shader.getImmediate("position1");
			const GxImmediateIndex position2_idx = shader.getImmediate("position2");
			
			for (int i = 0; i < 100; ++i)
			{
				const float x = mouse.x + cosf(framework.time * i / 100.f) * i * 2.f;
				const float y = mouse.y + sinf(framework.time * i / 100.f) * i * 2.f;
				
				shader.setImmediate(position1_idx, 10, 10);
				shader.setImmediate(position2_idx, x, y);
				{
					gxEmitVertices(GX_TRIANGLE_STRIP, 1000);
				}
			}
			clearShader();
		}
		framework.endDraw();
	}

	framework.shutdown();

	return 0;
}
