#include "audioNodeJgmod.h"
#include "jgmod.h"

#include "allegro2-timerApi.h"
#include "allegro2-voiceApi.h"

#include "soundmix.h"
#include "StringEx.h"

#include "ObjectLinkage.h"
DefineLinkage(jgmod_audiograph);

AUDIO_NODE_TYPE(AudioNodeJgmod)
{
	typeName = "jgmod";
	
	in("filename", "string");
	in("speed", "audioValue", "1");
	in("pitch", "audioValue", "1");
	in("gain", "audioValue", "1");
	
	for (int i = 0; i < AudioNodeJgmod::kNumChannels; ++i)
	{
		char name[16];
		sprintf_s(name, sizeof(name), "ch%d", i + 1);
		
		out(name, "audioValue");
	}
}

AudioNodeJgmod::AudioNodeJgmod()
	: AudioNodeBase()
	, timerApi(nullptr)
	, voiceApi(nullptr)
	, jgmod(nullptr)
	, jgmodPlayer(nullptr)
	, currentFilename()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kAudioPlugType_String);
	addInput(kInput_Speed, kAudioPlugType_FloatVec);
	addInput(kInput_Pitch, kAudioPlugType_FloatVec);
	addInput(kInput_Gain, kAudioPlugType_FloatVec);
	
	for (int i = 0; i < kNumChannels; ++i)
		addOutput(kOutput_Channel1 + i, kAudioPlugType_FloatVec, &output[i]);
}

AudioNodeJgmod::~AudioNodeJgmod()
{
	freeJgmod();
}

void AudioNodeJgmod::updateJgmod()
{
	const char * filename = getInputString(kInput_Filename, nullptr);
	
	if (filename == nullptr)
	{
		freeJgmod();
	}
	else if (filename != currentFilename)
	{
		freeJgmod();
		
		//
		
		currentFilename = filename;
		
		timerApi = new AllegroTimerApi(AllegroTimerApi::kMode_Manual);
		voiceApi = new AllegroVoiceApi(SAMPLE_RATE, false);
		
		jgmod = jgmod_load(filename);
		
		jgmodPlayer = new JGMOD_PLAYER();
		jgmodPlayer->init(32, timerApi, voiceApi);
		jgmodPlayer->play(jgmod, true);
	}
}

void AudioNodeJgmod::freeJgmod()
{
	currentFilename.clear();
	
	if (jgmodPlayer != nullptr)
	{
		jgmodPlayer->stop();
		
		delete jgmodPlayer;
		jgmodPlayer = nullptr;
	}
	
	if (jgmod != nullptr)
	{
		jgmod_destroy(jgmod);
		jgmod = nullptr;
	}
	
	delete timerApi;
	timerApi = nullptr;
	
	delete voiceApi;
	voiceApi = nullptr;
}

void AudioNodeJgmod::tick(const float dt)
{
	if (isPassthrough)
	{
		freeJgmod();
		
		for (int i = 0; i < kNumChannels; ++i)
			output[i].setZero();
		
		return;
	}
	
	//
	
	updateJgmod();
	
	//
	
	if (jgmod == nullptr)
	{
		for (int i = 0; i < kNumChannels; ++i)
			output[i].setZero();
		
		return;
	}
	
	const float speed = getInputAudioFloat(kInput_Speed, &AudioFloat::One)->getMean();
	const float pitch = getInputAudioFloat(kInput_Pitch, &AudioFloat::One)->getMean();
	const float gain = getInputAudioFloat(kInput_Gain, &AudioFloat::One)->getMean();
	
	jgmodPlayer->set_speed(speed * 100.f);
	jgmodPlayer->set_pitch(pitch * 100.f);
	
	timerApi->processInterrupts(dt * 1000000.f);
	
	for (int i = 0; i < jgmod->no_chn && i < kNumChannels; ++i)
	{
		output[i].setVector();
	
		float stereoPanning;
		
		voiceApi->generateSamplesForVoice(i, output[i].samples, AUDIO_UPDATE_SIZE, stereoPanning);
		
		audioBufferMul(output[i].samples, AUDIO_UPDATE_SIZE, gain);
	}
	
	for (int i = jgmod->no_chn; i < kNumChannels; ++i)
		output[i].setZero();
}

void AudioNodeJgmod::init(const GraphNode & node)
{
	updateJgmod();
}

void linkAudioNodes_Jgmod()
{
}
