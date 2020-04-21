#include "framework.h"

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(400, 600))
		return -1;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			const float temp1 = 1000.f;
			const float temp2 = 12000.f; // (1000 + 12000) / 2 = 6500 Kelvin in the middle of the screen
			
			// fyi: the sun has a blackbody temperature of around 5780 Kelvin
			
			for (int i = 0; i < 400; ++i)
			{
				const float t = (i + .5f) / 400.f;
				const float temp = lerp<float>(temp1, temp2, t);
				
				const float x1 = (i + 0) / 400.f * 400;
				const float x2 = (i + 1) / 400.f * 400;
				const float y1 = 10;
				const float y2 = 60;
				
				setShader_ColorTemperature(0, temp, 1.f);
				setColor(colorWhite);
				drawRect(x1, y1, x2, y2);
				clearShader();
			}
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
