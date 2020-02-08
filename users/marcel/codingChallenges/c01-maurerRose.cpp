#include "framework.h"
#include "nanovg-canvas.h"

using namespace NvgCanvasFunctions;

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	const int sx = 800;
	const int sy = 600;
	
	framework.enableDepthBuffer = true;
	
	if (!framework.init(sx, sy))
		return -1;
	
	float d = 71;

	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		float n = fmodf(framework.time, d);
		
		framework.beginDraw(255, 255, 255, 255);
		{
			beginDraw();
			
			strokeJoin(StrokeJoin::Round);
			
			beginStroke(0, 0, 0);
			{
				strokeWeight(.5f);
				
				for (int theta = 0; theta <= 360; ++theta)
				{
					const float k = theta * d * float(M_PI) / 180;
					const float r = 300 * sinf(int(n) * k);
					const float x = r * cosf(k) + sx/2;
					const float y = r * sinf(k) + sy/2;
					
					vertex(x, y);
				}
			}
			endStroke();

			beginShape();
			{
				fill(63, 127, 255, 30);
				stroke(63, 127, 255, 63);
				strokeWeight(4.f);
				strokeJoin(StrokeJoin::Round);
				
				for (int theta = 0; theta <= 360; ++theta)
				{
					const float k = theta * float(M_PI) / 180;
					const float r = 300 * sinf(n * k);
					const float x = r * cosf(k) + sx/2;
					const float y = r * sinf(k) + sy/2;
					
					vertex(x, y);
				}
			}
			endShape();
			
			endDraw();
		}
		framework.endDraw();
	}
	
	framework.shutdown();
	
	return 0;
}
