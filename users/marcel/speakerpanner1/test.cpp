#include "framework.h"
#include "imgui-framework.h"
#include "panner.h"

int main(int argc, char * argv[])
{
	if (!framework.init(800, 600))
		return -1;
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	bool showGui = true;
	
	SpeakerPanning::Panner_Grid panner;
	SpeakerPanning::GridDescription gridDescription;
	gridDescription.size[0] = 2;
	gridDescription.size[1] = 2;
	gridDescription.size[2] = 2;
	gridDescription.min.Set(-2.f, -2.f, -2.f);
	gridDescription.max.Set(+2.f, +2.f, +2.f);
	panner.init(gridDescription);
	
	SpeakerPanning::Source source1;
	//SpeakerPanning::Source source2;
	
	panner.addSource(&source1);
	//panner.addSource(&source2);
	
	Camera3d camera;
	camera.position.Set(0.f, 1.f, -2.f);
	camera.pitch = 15.f;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		bool inputIsCaptured = false;
		
		guiContext.processBegin(framework.timeStep, 800, 600, inputIsCaptured);
		{
			if (showGui)
			{
				if (ImGui::Begin("Monitor"))
				{
					ImGui::Checkbox("Apply constant power curve", &panner.applyConstantPowerCurve);
				}
				ImGui::End();
			}
		}
		guiContext.processEnd();
		
		if (inputIsCaptured == false)
		{
			if (keyboard.wentDown(SDLK_TAB))
			{
				inputIsCaptured = true;
				
				showGui = !showGui;
			}
		}
		
		camera.tick(framework.timeStep, showGui == false);
		
		source1.position[0] = cosf(framework.time / 1.23f) * 1.8f;
		source1.position[1] = sinf(framework.time / 1.34f) * 1.8f;
		source1.position[2] = sinf(framework.time / 1.45f) * 1.8f;
		
		panner.updatePanning();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .01f, 100.f);
			camera.pushViewMatrix();
			
			setColor(colorWhite);
			lineCube(
				(gridDescription.min + gridDescription.max) / 2.f,
				(gridDescription.max - gridDescription.min) / 2.f);
			
			setColor(colorGreen);
			fillCube(source1.position, Vec3(.02f, .02f, .02f));
			
			auto & source_elem = panner.sources[0];
			
			pushBlend(BLEND_ADD);
			{
				for (int i = 0; i < 8; ++i)
				{
					const int speakerIndex = source_elem.panning[i].speakerIndex;
					const Vec3 speakerPosition = panner.calculateSpeakerPosition(speakerIndex);
					
					setColorf(1, 1, 1, source_elem.panning[i].amount);
					fillCube(speakerPosition, Vec3(.1f, .1f, .1f));
				}
			}
			popBlend();
			
			camera.popViewMatrix();
			
			projectScreen2d();
			
			guiContext.draw();
		}
		framework.endDraw();
	}
	
	guiContext.shut();
	
	framework.shutdown();
	
	return 0;
}
