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
#include "framework.h"
#include "graph.h"
#include "portaudio/portaudio.h"
#include "soundmix.h"
#include "../libparticle/ui.h"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1300;
const int GFX_SY = 800;

//

#define USE_AUDIO_GRAPH 1

//

static SDL_mutex * mutex = nullptr;

//

struct PortAudioObject
{
	PaStream * stream;
	
	PortAudioObject()
		: stream(nullptr)
	{
	}
	
	~PortAudioObject()
	{
		shut();
	}
	
	bool init(AudioSource * audioSource);
	bool initImpl(AudioSource * audioSource);
	bool shut();
};

static int portaudioCallback(
	const void * inputBuffer,
	      void * outputBuffer,
	unsigned long framesPerBuffer,
	const PaStreamCallbackTimeInfo * timeInfo,
	PaStreamCallbackFlags statusFlags,
	void * userData)
{
	//logDebug("portaudioCallback!");
	
	Assert(framesPerBuffer == AUDIO_UPDATE_SIZE);
	
	AudioSource * audioSource = (AudioSource*)userData;
	
	float * __restrict destinationBuffer = (float*)outputBuffer;
	
	audioSource->generate(destinationBuffer, AUDIO_UPDATE_SIZE);

	return paContinue;
}

bool PortAudioObject::init(AudioSource * audioSource)
{
	if (initImpl(audioSource) == false)
	{
		shut();
		
		return false;
	}
	else
	{
		return true;
	}
}

bool PortAudioObject::initImpl(AudioSource * audioSource)
{
	PaError err;
	
	if ((err = Pa_Initialize()) != paNoError)
	{
		logError("portaudio: failed to initialize: %s", Pa_GetErrorText(err));
		return false;
	}
	
	logDebug("portaudio: version=%d, versionText=%s", Pa_GetVersion(), Pa_GetVersionText());
	
#if 0
	const int numDevices = Pa_GetDeviceCount();
	
	for (int i = 0; i < numDevices; ++i)
	{
		const PaDeviceInfo * deviceInfo = Pa_GetDeviceInfo(i);
	}
#endif
	
	PaStreamParameters outputParameters;
	memset(&outputParameters, 0, sizeof(outputParameters));
	
	outputParameters.device = Pa_GetDefaultOutputDevice();
	
	if (outputParameters.device == paNoDevice)
	{
		logError("portaudio: failed to find output device");
		return false;
	}
	
	outputParameters.channelCount = 1;
	outputParameters.sampleFormat = paFloat32;
	outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
	outputParameters.hostApiSpecificStreamInfo = nullptr;
	
	if ((err = Pa_OpenStream(&stream, nullptr, &outputParameters, SAMPLE_RATE, AUDIO_UPDATE_SIZE, paDitherOff, portaudioCallback, audioSource)) != paNoError)
	{
		logError("portaudio: failed to open stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	if ((err = Pa_StartStream(stream)) != paNoError)
	{
		logError("portaudio: failed to start stream: %s", Pa_GetErrorText(err));
		return false;
	}
	
	return true;
}

bool PortAudioObject::shut()
{
	PaError err;
	
	if (stream != nullptr)
	{
		if (Pa_IsStreamActive(stream) != 0)
		{
			if ((err = Pa_StopStream(stream)) != paNoError)
			{
				logError("portaudio: failed to stop stream: %s", Pa_GetErrorText(err));
				return false;
			}
		}
		
		if ((err = Pa_CloseStream(stream)) != paNoError)
		{
			logError("portaudio: failed to close stream: %s", Pa_GetErrorText(err));
			return false;
		}
		
		stream = nullptr;
	}
	
	if ((err = Pa_Terminate()) != paNoError)
	{
		logError("portaudio: failed to shutdown: %s", Pa_GetErrorText(err));
		return false;
	}
	
	return true;
}

//

struct AudioSource4DWorld : AudioSource
{
	double time;
	
	PcmData sound1;
	PcmData sound2;
	
	AudioSourcePcm pcm1;
	AudioSourcePcm pcm2;
	AudioSourceSine sine;
	AudioSourceMix mix;
	
	void init()
	{
		sound1.load("sound.ogg", 0);
		sound2.load("sound.ogg", 0);
		//sound2.load("music2.ogg", 0);
		
		pcm1.init(&sound1, 0);
		pcm1.play();
		pcm2.init(&sound2, 0);
		pcm2.play();
		sine.init(0.f, 400.f);
		//mix.add(&pcm1, 1.f);
		mix.add(&pcm2, 1.f);
		//mix.add(&sine, 1.f);
	}
	
	void tick(const double dt)
	{
		time += dt;
	}
	
	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) override
	{
		const double dt = numSamples / double(SAMPLE_RATE);
		
		tick(dt);
		
		pcm1.setRangeNorm((std::sin(time / 1.234) + 1.0) * 0.5, std::fmod(time / 1.345, 1.0));
		pcm2.setRangeNorm((std::sin(time / 12.345) + 1.0) * 0.5, std::fmod(time / 0.345, 1.0));
		//pcm2.setRange(pcm2.samplePosition % pcm2.pcmData->numSamples, int(time * 20.0) % (AUDIO_UPDATE_SIZE * 2));
		
		mix.generate(samples, numSamples);
	}
};

//

struct AudioSourceAudioGraph : AudioSource
{
	AudioGraph * audioGraph;
	
	AudioSourceAudioGraph()
		: audioGraph(nullptr)
	{
	}
	
	virtual void generate(ALIGN16 float * __restrict samples, const int numSamples) override
	{
		if (audioGraph == nullptr)
		{
			for (int i = 0; i < numSamples; ++i)
				samples[i] = 0.f;
		}
		else
		{
			SDL_LockMutex(mutex);
			{
				Assert(numSamples == AUDIO_UPDATE_SIZE);
				AudioBuffer & audioBuffer = *(AudioBuffer*)samples;
				
				audioGraph->tick(AUDIO_UPDATE_SIZE / double(SAMPLE_RATE));
				audioGraph->draw(audioBuffer);
			}
			SDL_UnlockMutex(mutex);
		}
	}
};

//

int main(int argc, char * argv[])
{
	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		initUi();
		
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
			typeDefinition.visualizer = "valueplotter";
			typeDefinitionLibrary.valueTypeDefinitions[typeDefinition.typeName] = typeDefinition;
		}
		
		createAudioEnumTypeDefinitions(typeDefinitionLibrary, g_audioEnumTypeRegistrationList);
		createAudioNodeTypeDefinitions(typeDefinitionLibrary, g_audioNodeTypeRegistrationList);
		
		GraphEdit graphEdit(&typeDefinitionLibrary);
		
		AudioGraph * audioGraph = new AudioGraph();
		
		AudioRealTimeConnection realTimeConnection;
		realTimeConnection.audioGraph = audioGraph;
		realTimeConnection.audioGraphPtr = &audioGraph;
		
		graphEdit.realTimeConnection = &realTimeConnection;
		
		g_currentAudioGraph = realTimeConnection.audioGraph;
		
		graphEdit.load("audioGraph.xml");
		
		g_currentAudioGraph = nullptr;
		
	#if USE_AUDIO_GRAPH
		AudioSourceAudioGraph audioSource;
		
		audioSource.audioGraph = audioGraph;
		
		PortAudioObject pa;
		
		pa.init(&audioSource);
	#else
		AudioSource4DWorld world;
		
		world.init();
		
		PortAudioObject pa;
		
		pa.init(&world);
	#endif
		
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
				SDL_LockMutex(mutex);
				{
					g_currentAudioGraph = realTimeConnection.audioGraph;
					
					graphEdit.tick(dt);
					
					g_currentAudioGraph = nullptr;
					
					audioSource.audioGraph = audioGraph;
				}
				SDL_UnlockMutex(mutex);
			}
			
			//

			framework.beginDraw(0, 0, 0, 0);
			{
				graphEdit.draw();
				
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
		
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
		
		shutUi();
		
		framework.shutdown();
	}

	return 0;
}
