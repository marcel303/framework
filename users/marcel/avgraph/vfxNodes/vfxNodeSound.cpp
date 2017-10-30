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

#include "audiostream/AudioOutput.h"
#include "audiostream/AudioStreamVorbis.h"
#include "vfxNodeSound.h"
#include <cmath>

// todo : trigger on start to send beat 0

VFX_NODE_TYPE(sound, VfxNodeSound)
{
	typeName = "sound";
	
	in("source", "string");
	in("autoplay", "bool", "1");
	in("loop", "bool", "1");
	in("bpm", "float", "60");
	in("volume", "float", "1");
	in("play!", "trigger");
	in("stop!", "trigger");
	in("restart!", "trigger");
	in("pause!", "trigger");
	in("resume!", "trigger");
	out("time", "float");
	out("play!", "trigger");
	out("pause!", "trigger");
	out("beat!", "trigger");
	out("beatCount", "int");
}

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
	, timeOutput(0.f)
	, audioOutput(nullptr)
	, audioStream(nullptr)
	, isPaused(false)
	, outputBeatCount(0)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Source, kVfxPlugType_String);
	addInput(kInput_AutoPlay, kVfxPlugType_Bool);
	addInput(kInput_Loop, kVfxPlugType_Bool);
	addInput(kInput_BPM, kVfxPlugType_Float);
	addInput(kInput_Volume, kVfxPlugType_Float);
	addInput(kInput_Play, kVfxPlugType_Trigger);
	addInput(kInput_Stop, kVfxPlugType_Trigger);
	addInput(kInput_Restart, kVfxPlugType_Trigger);
	addInput(kInput_Pause, kVfxPlugType_Trigger);
	addInput(kInput_Resume, kVfxPlugType_Trigger);
	addOutput(kOutput_Time, kVfxPlugType_Float, &timeOutput);
	addOutput(kOutput_Play, kVfxPlugType_Trigger, nullptr);
	addOutput(kOutput_Pause, kVfxPlugType_Trigger, nullptr);
	addOutput(kOutput_Beat, kVfxPlugType_Trigger, nullptr);
	addOutput(kOutput_BeatCount, kVfxPlugType_Int, &outputBeatCount);
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
	vfxCpuTimingBlock(VfxNodeSound);
	
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
				outputBeatCount = beat2;
				
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

void VfxNodeSound::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_Play)
	{
		if (audioStream->IsOpen_get() == false)
		{
			const char * source = getInputString(kInput_Source, "");
			const bool loop = getInputBool(kInput_Loop, true);
			
			audioStream->Open(source, loop);
		}
		
		isPaused = false;
		
		trigger(kOutput_Play);
	}
	else if (inputSocketIndex == kInput_Stop)
	{
		audioStream->Close();
	}
	else if (inputSocketIndex == kInput_Restart)
	{
		const char * source = getInputString(kInput_Source, "");
		const bool loop = getInputBool(kInput_Loop, true);
		
		audioStream->Close();
		
		audioStream->Open(source, loop);
		
		isPaused = false;
		
		trigger(kOutput_Play);
	}
	else if (inputSocketIndex == kInput_Pause)
	{
		isPaused = true;
		
		trigger(kOutput_Pause);
	}
	else if (inputSocketIndex == kInput_Resume)
	{
		isPaused = false;
		
		trigger(kOutput_Play);
	}
}
