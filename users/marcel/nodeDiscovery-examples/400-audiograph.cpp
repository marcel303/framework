#include "audioGraph.h"
#include "audioGraphContext.h"
#include "audioGraphManager.h"
#include "audioVoiceManager.h"
#include "framework.h"
#include "graph.h"
#include "graph_typeDefinitionLibrary.h"

#include "audioStreamToTcp.h"
#include "nodeDiscovery.h"

/*

This code demoes a little step sequencer. There are four timelines, one for each audio channel.
Audio is generated through audio graphs. The sequencer automatically connects to the first node
it discovers with quad-channel audio support. The user can click on one of the boxes to
activate/inactivate notes.

*/

int main()
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	if (!framework.init(800, 600))
		return -1;
	
	// start the node discovery process
	
	NodeDiscoveryProcess discoveryProcess;
	discoveryProcess.init();
	
	// load the audio graph for the sound effect
	
	auto * types = createAudioTypeDefinitionLibrary();
	
	const char * filename = "400-audiograph.xml";
	
	Graph graph;
	graph.load(filename, types);
	
	// per-channel audio state
	
	static const int kNumChannels = 4;
	static const int kNumSteps = 8;
	
	struct AudioChannelState
	{
		AudioVoiceManagerBasic audioVoiceMgr;
		AudioGraphContext audioGraphContext;
		AudioGraph * audioGraph[kNumSteps] = { };
		
		void load(const Graph & graph, const Graph_TypeDefinitionLibrary * types)
		{
			unload();
			
			for (int i = 0; i < kNumSteps; ++i)
			{
				audioGraph[i] = constructAudioGraph(graph, types, &audioGraphContext, false);
			}
		}
		
		void unload()
		{
			for (int i = 0; i < kNumSteps; ++i)
			{
				delete audioGraph[i];
				audioGraph[i] = nullptr;
			}
		}
	};
	
	AudioChannelState audioChannelState[kNumChannels];
	
	// setup audio graphs
	
	AudioMutex audioMutex;
	audioMutex.init();
	
	for (int i = 0; i < kNumChannels; ++i)
	{
		audioChannelState[i].audioVoiceMgr.init(&audioMutex, kNumSteps);
		audioChannelState[i].audioGraphContext.init(
			&audioMutex,
			&audioMutex,
			&audioChannelState[i].audioVoiceMgr);
		audioChannelState[i].load(graph, types);
	}
	
	// setup grid
	
	struct Cell
	{
		bool active = false;
		float triggerTimer = 0.f;
	};
	
	Cell grid[kNumChannels][kNumSteps];
	
	float stepInterval = .3f;
	float stepTimer = 0.f;
	int nextStep = 0;
	
	// audio streamer
	
	AudioStreamToTcp audioStreamToTcp;
	auto audioFunction = [&](void * __restrict samples, const int numFrames, const int numChannels)
		{
			for (int c = 0; c < kNumChannels; ++c)
				for (int s = 0; s < kNumSteps; ++s)
					audioChannelState[c].audioGraph[s]->tickAudio(numFrames / 48000.f, true);
			
			float voice[kNumChannels][AUDIO_UPDATE_SIZE];
			for (int c = 0; c < numChannels; ++c)
				audioChannelState[c].audioVoiceMgr.generateAudio(voice[c], numFrames, 1);
			
		#if 1
			float * __restrict samples_float = (float*)samples;
			
			for (int i = 0; i < numFrames; ++i)
				for (int c = 0; c < numChannels; ++c)
					samples_float[i * numChannels + c] = voice[c][i];
		#else
			static float phase = 0.f;
			const float pitch = lerp(10.f, 1000.f, inverseLerp(0, 800, mouse.x));
			const float phaseStep = pitch / 44100.f;
			
			float value[kNumChannels] = { };
			for (int c = 0; c < numChannels; ++c)
				for (int s = 0; s < kNumSteps; ++s)
					value[c] = fmaxf(value[c], grid[c][s].triggerTimer);
			
			float * __restrict samples_float = (float*)samples;
			
			for (int i = 0; i < numFrames; ++i)
			{
			#if 0
				for (int c = 0; c < numChannels; ++c)
					samples_float[i * numChannels + c] = value;
			#elif 1
				const float wave = sinf(phase * float(M_PI));
				phase += phaseStep;
				if (phase > 1.f)
					phase -= 1.f;
				
				for (int c = 0; c < numChannels; ++c)
					samples_float[i * numChannels + c] = value[c] * wave;
			#else
				for (int c = 0; c < numChannels; ++c)
					samples_float[i * numChannels + c] = voice[c][i];
			#endif
			}
		#endif
		};
	
	NodeDiscoveryRecord record;
	
	for (;;)
	{
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		if (framework.fileHasChanged(filename))
		{
			graph = Graph();
			graph.load(filename, types);
			for (int i = 0; i < kNumChannels; ++i)
				audioChannelState[i].load(graph, types);
		}
		
		if (record.isValid() == false)
		{
			for (int i = 0; i < discoveryProcess.getRecordCount(); ++i)
			{
				auto r = discoveryProcess.getDiscoveryRecord(i);
				
				if (r.capabilities & kNodeCapability_TcpToI2SQuad)
				{
					record = r;
					
					audioStreamToTcp.beginShutdown();
					audioStreamToTcp.waitForShutdown();
					audioStreamToTcp.init(
						r.endpointName.address,
						I2S_4CH_PORT,
						I2S_4CH_BUFFER_COUNT,
						AUDIO_UPDATE_SIZE > I2S_4CH_CHANNEL_COUNT
							? AUDIO_UPDATE_SIZE
							: I2S_4CH_CHANNEL_COUNT,
						I2S_4CH_CHANNEL_COUNT,
						AudioStreamToTcp::kSampleFormat_Float,
						AudioStreamToTcp::kSampleFormat_S16,
						audioFunction);
					
					break;
				}
			}
		}
		
		discoveryProcess.purgeStaleRecords(10);
		
		// update grid animations
		
		for (int c = 0; c < kNumChannels; ++c)
		{
			for (int s = 0; s < kNumSteps; ++s)
			{
				grid[c][s].triggerTimer = fmaxf(0.f, grid[c][s].triggerTimer - framework.timeStep / 1.f);
			}
		}
		
		// update step sequencer
		
		stepTimer = fmaxf(0.f, stepTimer - framework.timeStep);
		
		if (stepTimer == 0.f)
		{
			//logDebug("next step");
			
			stepTimer = stepInterval;
			
			// send trigger event to audio graphs
			
			for (int c = 0; c < kNumChannels; ++c)
			{
				if (grid[c][nextStep].active)
				{
					grid[c][nextStep].triggerTimer = 1.f;
					
					const float pitch = lerp(10.f, 1000.f, inverseLerp(0, 800, mouse.x));
					audioChannelState[c].audioGraph[nextStep]->setMemf("freq", pitch);
					audioChannelState[c].audioGraph[nextStep]->triggerEvent("begin");
				}
			}
			
			nextStep = (nextStep + 1) % kNumSteps;
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// ui: connect/disconnect option
			// four bars: one for each audio channel/instrument
			// toggle before bar: mute/unmute channel
			// click: toggle note
			
			setColor(100, 255, 100);
			const int x = 800 / kNumSteps * (((nextStep + kNumSteps - 1) % kNumSteps) + .5f);
			const int y = 40;
			fillCircle(x, y, 12, 10);
			
			for (int c = 0; c < kNumChannels; ++c)
			{
				const Color color = Color::fromHSL(.7f, .3f, (c % 2) == 0 ? .2f : .3f);
				setColor(color);
				
				const int y1 = 100 + 500 / kNumChannels * (c + 0);
				const int y2 = 100 + 500 / kNumChannels * (c + 1);
				drawRect(0, y1, 800, y2);
				
				for (int s = 0; s < kNumSteps; ++s)
				{
					const int x1 = 800 / kNumSteps * (s + 0);
					const int x2 = 800 / kNumSteps * (s + 1);
					
					setColor(100, 100, 100);
					drawRectLine(x1 + 10, y1 + 10, x2 -10, y2 - 10);
					
					if (mouse.wentDown(BUTTON_LEFT) &&
						mouse.x >= x1 &&
						mouse.y >= y1 &&
						mouse.x < x2 &&
						mouse.y < y2)
					{
						grid[c][s].active = !grid[c][s].active;
					}
					
					if (grid[c][s].active)
					{
						setColor(255, 255, 255, 100);
						drawRect(x1 + 10, y1 + 10, x2 -10, y2 - 10);
					}
					
					if (grid[c][s].active)
					{
						setColor(100, 255, 100);
						setAlphaf(grid[c][s].triggerTimer);
						drawRect(x1 + 10, y1 + 10, x2 -10, y2 - 10);
					}
				}
			}
			
			if (record.isValid())
			{
				drawText(4, 4, 18, +1, +1, "%s", record.description);
			}
		}
		framework.endDraw();
	}
	
	audioStreamToTcp.beginShutdown();
	audioStreamToTcp.waitForShutdown();
	
	for (int i = 0; i < kNumChannels; ++i)
	{
		audioChannelState[i].unload();
		audioChannelState[i].audioGraphContext.shut();
		audioChannelState[i].audioVoiceMgr.shut();
	}
	
	audioMutex.shut();
	
	delete types;
	types = nullptr;
	
	discoveryProcess.shut();
	
	framework.shutdown();
	
	return 0;
}
