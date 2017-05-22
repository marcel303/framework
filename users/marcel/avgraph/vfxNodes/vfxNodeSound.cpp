#include "audiostream/AudioOutput.h"
#include "audiostream/AudioStreamVorbis.h"
#include "framework.h"
#include "vfxNodeSound.h"

// todo : trigger on start to send beat 0

class AudioStreamNULL : public AudioStream
{
public:
	virtual int Provide(int numSamples, AudioSample * __restrict buffer) override
	{
		return 0;
	}
};

VfxNodeSound::VfxNodeSound()
	: VfxNodeBase()
	, playTrigger()
	, pauseTrigger()
	, timeOutput(0.f)
	, beatTrigger()
	, audioOutput(nullptr)
	, audioStream(nullptr)
	, isPaused(false)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addInput(kInput_AutoPlay, kVfxPlugType_Bool);
	addInput(kInput_Loop, kVfxPlugType_Bool);
	addInput(kInput_BPM, kVfxPlugType_Float);
	addInput(kInput_Volume, kVfxPlugType_Float);
	addInput(kInput_Play, kVfxPlugType_Trigger);
	addInput(kInput_Pause, kVfxPlugType_Trigger);
	addInput(kInput_Resume, kVfxPlugType_Trigger);
	addOutput(kOutput_Time, kVfxPlugType_Float, &timeOutput);
	addOutput(kOutput_Play, kVfxPlugType_Trigger, &playTrigger);
	addOutput(kOutput_Pause, kVfxPlugType_Trigger, &pauseTrigger);
	addOutput(kOutput_Beat, kVfxPlugType_Trigger, &beatTrigger);
}

VfxNodeSound::~VfxNodeSound()
{
	delete audioOutput;
	audioOutput = nullptr;
	
	delete audioStream;
	audioStream = nullptr;
}

void VfxNodeSound::tick(const float dt)
{
	const char * source = getInputString(kInput_Source, "");
	const bool loop = getInputBool(kInput_Loop, true);
	const float volume = getInputFloat(kInput_Volume, 1.f);
	
	if (strcmp(source, audioStream->FileName_get()) || loop != audioStream->Loop_get())
	{
		audioStream->Close();
		
		audioStream->Open(source, loop);
	}
	
	audioOutput->Volume_set(volume);
	
	if (audioOutput->IsPlaying_get())
	{
		if (audioStream->IsOpen_get() == false)
		{
			AudioStreamNULL nullStream;
				
			audioOutput->Update(&nullStream);
		}
		else
		{
			const double bpm = getInputFloat(kInput_BPM, 60.f);
			const double bps = bpm / 60.0;
			
			// update streaming
			
			const double t1 = audioOutput->PlaybackPosition_get();
			
			if (isPaused)
			{
				AudioStreamNULL nullStream;
				
				audioOutput->Update(&nullStream);
			}
			else
			{
				audioOutput->Update(audioStream);
			}
			
			const double t2 = audioOutput->PlaybackPosition_get();
			
			// update time
			
			const int beat1 = int(std::floor(timeOutput * bps));
			
			timeOutput = t2;
			
			const int beat2 = int(std::floor(timeOutput * bps));

			// check if we crossed a beat marker
			
			if (beat1 != beat2)
			{
				beatTrigger.setInt(beat2);
				trigger(kOutput_Beat);
			}
		}
	}
}

void VfxNodeSound::init(const GraphNode & node)
{
	const char * source = getInputString(kInput_Source, "");
	const bool autoPlay = getInputBool(kInput_AutoPlay, true);
	const bool loop = getInputBool(kInput_Loop, true);
	
	audioStream = new AudioStream_Vorbis();
	audioStream->Open(source, loop);
	
	audioOutput = new AudioOutput_OpenAL();
	audioOutput->Initialize(2, 44100, 4096);
	audioOutput->Play();
	
	if (autoPlay)
	{
		isPaused = false;
	}
	else
	{
		isPaused = true;
	}
}

void VfxNodeSound::handleTrigger(const int inputSocketIndex, const VfxTriggerData & data)
{
	if (inputSocketIndex == kInput_Play)
	{
		isPaused = false;
		
		playTrigger.setBool(true);
		trigger(kOutput_Play);
	}
	else if (inputSocketIndex == kInput_Pause)
	{
		isPaused = true;
		
		pauseTrigger.setBool(false);
		trigger(kOutput_Pause);
	}
	else if (inputSocketIndex == kInput_Resume)
	{
		isPaused = false;
		
		playTrigger.setBool(true);
		trigger(kOutput_Play);
	}
}
