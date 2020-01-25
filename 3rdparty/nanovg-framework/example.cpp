#include "framework.h"
#include "nanovg.h"
#include "nanovg-framework.h"

#include "demo.h"
#include "perf.h"

int main(int argc, char * argv[])
{
	changeDirectory(CHIBI_RESOURCE_PATH);
	
	framework.enableDepthBuffer = true; // required for stencil operations performed by NanoVG
	framework.allowHighDpi = true;
	
	if (!framework.init(1200, 600))
		return -1;

	auto * vg = nvgCreateFramework(
		NVG_ANTIALIAS |
		NVG_STENCIL_STROKES |
		NVG_DEBUG);
	
	DemoData data;
	
	if (loadDemoData(vg, &data) == -1)
		return -1;
	
	PerfGraph fps;
	
	bool blowup = false;
	
	for (;;)
	{
		framework.process();

		if (framework.quitRequested)
			break;

		if (keyboard.wentDown(SDLK_SPACE))
			blowup = !blowup;
		
		updateGraph(&fps, framework.timeStep);
		
		framework.beginDraw(
			0.3f  * 255,
			0.3f  * 255,
			0.32f * 255,
			1.0f  * 255);
		{
			int sx;
			int sy;
			framework.getCurrentViewportSize(sx, sy);
			
			nvgBeginFrame(vg, sx, sy, framework.getCurrentBackingScale());

			renderDemo(vg, mouse.x, mouse.y, sx, sy, framework.time, blowup, &data);
			renderGraph(vg, 5, 5, &fps);

			nvgEndFrame(vg);
		}
		framework.endDraw();
	}
	
	freeDemoData(vg, &data);
	
	nvgDeleteFramework(vg);

	framework.shutdown();

	return 0;
}
