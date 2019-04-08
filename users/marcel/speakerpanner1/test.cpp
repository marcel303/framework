#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioVoiceManager.h"
#include "framework.h"
#include "imgui-framework.h"
#include "panner.h"
#include "ui.h"

struct SoundObject : SpatialSound::Source
{
	Color color = colorWhite;
	
	AudioGraphInstance * graphInstance = nullptr;
};

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	changeDirectory(CHIBI_RESOURCE_PATH);
#endif
	
	if (!framework.init(800, 600))
		return -1;
	
	initUi();
	
	FrameworkImGuiContext guiContext;
	guiContext.init();
	
	bool showGui = true;
	
	SpeakerPanning::Panner_Grid panner;
	SpeakerPanning::GridDescription gridDescription;
	gridDescription.size[0] = 8;
	gridDescription.size[1] = 8;
	gridDescription.size[2] = 8;
	gridDescription.min.Set(-2.f, -2.f, -2.f);
	gridDescription.max.Set(+2.f, +2.f, +2.f);
	panner.init(gridDescription);
	
	std::vector<SoundObject> soundObjects;
	soundObjects.resize(1000);
	for (size_t i = 0; i < soundObjects.size(); ++i)
	{
		auto & soundObject = soundObjects[i];
		const float hue = i / float(soundObjects.size());
		soundObject.color = Color::fromHSL(hue, .5f, .5f);
		
		auto & source = soundObject;
		panner.addSource(&source);
	}
	
	AudioMutex audioMutex;
	audioMutex.init();
	
	AudioVoiceManagerBasic audioVoiceMgr;
	audioVoiceMgr.init(audioMutex.mutex, 256, 256);
	
	AudioGraphManager_RTE audioGraphMgr(800, 600);
	audioGraphMgr.init(audioMutex.mutex, &audioVoiceMgr);
	
	auto * instance = audioGraphMgr.createInstance("soundObject1.xml");
	audioGraphMgr.selectInstance(instance);
	
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
		
		audioGraphMgr.tickMain();
		
		inputIsCaptured |= audioGraphMgr.tickEditor(800, 600, framework.timeStep, inputIsCaptured);
		
		camera.tick(framework.timeStep, showGui == false);
		
		for (size_t i = 0; i < soundObjects.size(); ++i)
		{
			auto & source = soundObjects[i];
			
			const float speed = 1.f + i / float(soundObjects.size()) * 10.f;
			
			source.position[0] = cosf(framework.time / 1.23f * speed) * 1.8f;
			source.position[1] = sinf(framework.time / 1.34f * speed) * 1.8f;
			source.position[2] = sinf(framework.time / 1.45f * speed) * 1.8f;
		}
		
		panner.updatePanning();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			projectPerspective3d(60.f, .01f, 100.f);
			camera.pushViewMatrix();
			
			setColor(colorWhite);
			lineCube(
				(gridDescription.min + gridDescription.max) / 2.f,
				(gridDescription.max - gridDescription.min) / 2.f);
			
			beginCubeBatch();
			{
				for (auto & soundObject : soundObjects)
				{
					setColor(colorGreen);
					fillCube(soundObject.position, Vec3(.02f, .02f, .02f));
				}
			}
			endCubeBatch();
			
			pushBlend(BLEND_ADD);
			beginCubeBatch();
			{
				for (auto & soundObject : soundObjects)
				{
					auto & source = soundObject;
					auto & source_elem = panner.getSourceElemForSource(&source);
					
					for (int i = 0; i < 8; ++i)
					{
						const int speakerIndex = source_elem.panning[i].speakerIndex;
						const Vec3 speakerPosition = panner.calculateSpeakerPosition(speakerIndex);
						
						setColor(source.color);
						setAlphaf(source_elem.panning[i].amount);
						fillCube(speakerPosition, Vec3(.1f, .1f, .1f));
					}
				}
			}
			endCubeBatch();
			popBlend();
			
			camera.popViewMatrix();
			
			projectScreen2d();
			
			audioGraphMgr.drawEditor(800, 600);
			
			guiContext.draw();
		}
		framework.endDraw();
	}
	
	audioGraphMgr.free(instance, false);
	
	audioGraphMgr.shut();
	audioVoiceMgr.shut();
	audioMutex.shut();
	
	for (auto & soundObject : soundObjects)
		panner.removeSource(&soundObject);
	soundObjects.clear();
	
	guiContext.shut();
	
	shutUi();
	
	framework.shutdown();
	
	return 0;
}
