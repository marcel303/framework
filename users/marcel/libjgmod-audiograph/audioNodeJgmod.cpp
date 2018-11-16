#include "audioNodeJgmod.h"
#include "jgmod.h"

#include "framework-allegro2.h"

#include "soundmix.h"

AUDIO_NODE_TYPE(AudioNodeJgmod)
{
	typeName = "jgmod";
	
	in("filename", "string");
	in("speed", "float", "1");
	in("pitch", "float", "1");
	in("gain", "float", "1");
	
	for (int i = 0; i < AudioNodeJgmod::kNumChannels; ++i)
	{
		char name[16];
		sprintf(name, "ch%d", i + 1);
		
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
	addInput(kInput_Speed, kAudioPlugType_Float);
	addInput(kInput_Pitch, kAudioPlugType_Float);
	addInput(kInput_Gain, kAudioPlugType_Float);
	
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
	
	delete jgmodPlayer;
	jgmodPlayer = nullptr;
	
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
	
	const float speed = getInputFloat(kInput_Speed, 1.f);
	const float pitch = getInputFloat(kInput_Pitch, 1.f);
	const float gain = getInputFloat(kInput_Gain, 1.f);
	
	jgmodPlayer->set_speed(speed * 100.f);
	jgmodPlayer->set_pitch(pitch * 100.f);
	
	timerApi->processInterrupts(dt * 1000000.f);
	
	for (int i = 0; i < jgmod->no_chn; ++i)
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
