/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "audioGraph.h"
#include "audioGraphRealTimeConnection.h"
#include "audioNodeBase.h"
#include "audioNodes/audioNodeDisplay.h"
#include "framework.h"
#include "graph.h"
#include "paobject.h"
#include "soundmix.h"
#include "../libparticle/ui.h"

#define FULLSCREEN 0

extern const int GFX_SX;
extern const int GFX_SY;

#if FULLSCREEN
	const int GFX_SX = 2560/2;
	const int GFX_SY = 1600/2;
#else
	const int GFX_SX = 1300;
	const int GFX_SY = 800;
#endif

//

//#define FILENAME "audioGraph.xml"
#define FILENAME "audioTest1.xml"

//

static SDL_mutex * mutex = nullptr;
static GraphEdit * graphEdit = nullptr;
static AudioRealTimeConnection * realTimeConnection = nullptr;

//

struct AudioSourceAudioGraph : PortAudioHandler
{
	AudioGraph ** audioGraphPtr;
	
	AudioSourceAudioGraph()
		: audioGraphPtr(nullptr)
	{
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		void * outputBuffer,
		int framesPerBuffer)
	{
		Assert(framesPerBuffer == AUDIO_UPDATE_SIZE);

		float * samples = (float*)outputBuffer;
		const int numSamples = framesPerBuffer;

		if (*audioGraphPtr == nullptr)
		{
			for (int i = 0; i < numSamples * 2; ++i)
				samples[i] = 0.f;
		}
		else
		{
			SDL_LockMutex(mutex);
			{
				Assert(numSamples == AUDIO_UPDATE_SIZE);
				
				const double dt = AUDIO_UPDATE_SIZE / double(SAMPLE_RATE);
				
				AudioOutputChannel channels[2];
				channels[0].samples = samples + 0;
				channels[0].stride = 2;
				channels[1].samples = samples + 1;
				channels[1].stride = 2;
				
				AudioGraph * audioGraph = *audioGraphPtr;
				audioGraph->tick(dt, true);
				audioGraph->draw(channels, 2, true);
				
				realTimeConnection->updateAudioValues();
			}
			SDL_UnlockMutex(mutex);
		}
	}
};

//

static AudioVoiceManager * g_voiceMgr = nullptr;

struct Creature
{
	AudioSourceSine sine;
	AudioVoice * voice;
	
	Creature()
		: sine()
		, voice(nullptr)
	{
	}
	
	~Creature()
	{
		shut();
	}
	
	void init()
	{
		sine.init(0.f, random(100.f, 400.f));
		
		g_voiceMgr->allocVoice(voice, &sine);
	}
	
	void shut()
	{
		if (voice != nullptr)
		{
			g_voiceMgr->freeVoice(voice);
		}
	}
};

struct World
{
	std::list<Creature> creatures;
	
	World()
		: creatures()
	{
	}
	
	void init(const int numCreatures)
	{
		for (int i = 0; i < numCreatures; ++i)
		{
			creatures.push_back(Creature());
			
			Creature & creature = creatures.back();
			
			creature.init();
		}
	}
	
	void addCreature()
	{
		creatures.push_back(Creature());
			
		Creature & creature = creatures.back();
		
		creature.init();
	}
	
	void removeCreature()
	{
		creatures.pop_front();
	}
};

static void testAudioVoiceManager()
{
	const int kNumChannels = 16;
	
	//
	
	AudioVoiceManager voiceMgr;
	
	voiceMgr.init(kNumChannels);
	
	voiceMgr.outputMono = true;
	
	g_voiceMgr = &voiceMgr;
	
	//
	
	PortAudioObject pa;
	
	pa.init(SAMPLE_RATE, 1, AUDIO_UPDATE_SIZE, &voiceMgr);
	
	//
	
	World * world = new World();
	
	world->init(1);
	
	//
	
	do
	{
		framework.process();
		
		//
		
		if (keyboard.wentDown(SDLK_a))
		{
			world->addCreature();
		}
		
		if (keyboard.wentDown(SDLK_z))
		{
			world->removeCreature();
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	//
	
	delete world;
	world = nullptr;
	
	//
	
	pa.shut();
	
	voiceMgr.shut();
}

//

int main(int argc, char * argv[])
{
#if FULLSCREEN
	framework.fullscreen = true;
#endif
	
	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		initUi();
		
		//
		
		testAudioVoiceManager();
		
		//
		
		mutex = SDL_CreateMutex();
		
		GraphEdit_TypeDefinitionLibrary typeDefinitionLibrary;
		
		{
			GraphEdit_ValueTypeDefinition typeDefinition;
			typeDefinition.typeName = "bool";
			typeDefinition.editor = "checkbox";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
		{
			GraphEdit_ValueTypeDefinition typeDefinition;
			typeDefinition.typeName = "int";
			typeDefinition.editor = "textbox_int";
			typeDefinition.visualizer = "valueplotter";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
		{
			GraphEdit_ValueTypeDefinition typeDefinition;
			typeDefinition.typeName = "float";
			typeDefinition.editor = "textbox_float";
			typeDefinition.visualizer = "valueplotter";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
		{
			GraphEdit_ValueTypeDefinition typeDefinition;
			typeDefinition.typeName = "string";
			typeDefinition.editor = "textbox";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
		{
			GraphEdit_ValueTypeDefinition typeDefinition;
			typeDefinition.typeName = "audioValue";
			typeDefinition.editor = "textbox_float";
			typeDefinition.visualizer = "channels";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
		createAudioEnumTypeDefinitions(typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
		createAudioNodeTypeDefinitions(typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
		
		graphEdit = new GraphEdit(&typeDefinitionLibrary);
		
		realTimeConnection = new AudioRealTimeConnection();
		
		AudioGraph * audioGraph = new AudioGraph();
		
		realTimeConnection->audioGraph = audioGraph;
		realTimeConnection->audioGraphPtr = &audioGraph;
		realTimeConnection->audioMutex = mutex;
		
		graphEdit->realTimeConnection = realTimeConnection;
		
		graphEdit->load(FILENAME);
		
		AudioSourceAudioGraph audioSource;
		
		audioSource.audioGraphPtr = &audioGraph;
		
		PortAudioObject pa;
		
		pa.init(SAMPLE_RATE, 2, AUDIO_UPDATE_SIZE, &audioSource);
		
		bool stop = false;
		
		do
		{
			framework.process();
			
			//
			
			const float dt = framework.timeStep;
			
			//
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;
			else
			{
				graphEdit->tick(dt);
			}
			
			//

			framework.beginDraw(0, 0, 0, 0);
			{
				graphEdit->draw();
				
				pushFontMode(FONT_SDF);
				{
					setFont("calibri.ttf");
					
					setColor(colorGreen);
					drawText(GFX_SX/2, 20, 20, 0, 0, "- 4DWORLD -");
				}
				popFontMode();
			}
			framework.endDraw();
		} while (stop == false);
		
		Font("calibri.ttf").saveCache();
		
		pa.shut();
		
		delete audioGraph;
		audioGraph = nullptr;
		realTimeConnection->audioGraph = nullptr;
		realTimeConnection->audioGraphPtr = nullptr;
		
		delete realTimeConnection;
		realTimeConnection = nullptr;
		
		delete graphEdit;
		graphEdit = nullptr;
		
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
