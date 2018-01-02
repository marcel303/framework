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

#include "audiostream/AudioOutput_PortAudio.h"
#include "audiostream/AudioStreamVorbis.h"
#include "vfxNodeSound.h"
#include <math.h>
#include <SDL2/SDL.h>

//

int VfxNodeSound_AudioStream::Provide(int numSamples, AudioSample * __restrict buffer)
{
	int result = 0;
	
	SDL_LockMutex(soundNode->mutex);
	{
		if (!soundNode->isPaused && soundNode->audioStream->IsOpen_get())
		{
			const int numSamplesRead = soundNode->audioStream->Provide(numSamples, buffer);
			
			timeInSamples += numSamplesRead;
			
			result = numSamplesRead;
		}
	}
	SDL_UnlockMutex(soundNode->mutex);
	
	return result;
}

// todo : trigger on start to send beat 0

VFX_NODE_TYPE(VfxNodeSound)
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

VfxNodeSound::VfxNodeSound()
	: VfxNodeBase()
	, mutex(nullptr)
	, audioOutput(nullptr)
	, audioStream(nullptr)
	, isPaused(false)
	, timeOutput(0.f)
	, beatCountOutput(0)
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
	addOutput(kOutput_BeatCount, kVfxPlugType_Int, &beatCountOutput);
	
	mutex = SDL_CreateMutex();
	Assert(mutex != nullptr);
	
	mixingAudioStream.soundNode = this;
}

VfxNodeSound::~VfxNodeSound()
{
	delete audioOutput;
	audioOutput = nullptr;
	
	delete audioStream;
	audioStream = nullptr;
	
	Assert(mutex != nullptr);
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
}

void VfxNodeSound::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeSound);
	
	if (isPassthrough)
	{
		delete audioOutput;
		audioOutput = nullptr;
		
		delete audioStream;
		audioStream = nullptr;
	
		mixingAudioStream.timeInSamples = 0;
		
		isPaused = false;
		
		timeOutput = 0.f;
		
		beatCountOutput = 0;
		
		return;
	}
	
	//
	
	const char * source = getInputString(kInput_Source, "");
	const bool loop = getInputBool(kInput_Loop, true);
	const bool autoPlay = getInputBool(kInput_AutoPlay, true);
	const float volume = getInputFloat(kInput_Volume, 1.f);
	
	if (audioStream == nullptr)
	{
		Assert(audioStream == nullptr);
		audioStream = new AudioStream_Vorbis();
		
		if (autoPlay)
			audioStream->Open(source, loop);
		
		Assert(mixingAudioStream.timeInSamples == 0);
		
		Assert(audioOutput == nullptr);
		audioOutput = new AudioOutput_PortAudio();
		audioOutput->Initialize(2, 44100, 256);
		audioOutput->Volume_set(volume);
		audioOutput->Play(&mixingAudioStream);
	}
	
	//
	
	if (audioStream->IsOpen_get() || autoPlay)
	{
		if (strcmp(source, audioStream->FileName_get()) != 0 || loop != audioStream->Loop_get())
		{
			SDL_LockMutex(mutex);
			{
				audioStream->Open(source, loop);
				
				mixingAudioStream.timeInSamples = 0;
			}
			SDL_UnlockMutex(mutex);
		}
	}
	
	audioOutput->Volume_set(volume);
	
	audioOutput->Update();
	
	{
		const double bpm = getInputFloat(kInput_BPM, 60.f);
		const double bps = bpm / 60.0;
		
		// update time
		
		const int beat1 = int(floor(timeOutput * bps));
		
		timeOutput = mixingAudioStream.timeInSamples / 44100.0;
		
		const int beat2 = int(floor(timeOutput * bps));

		// check if we crossed a beat marker
		
		if (beat1 != beat2)
		{
			beatCountOutput = beat2;
			
			trigger(kOutput_Beat);
		}
	}
}

void VfxNodeSound::init(const GraphNode & node)
{
	const char * source = getInputString(kInput_Source, "");
	const bool loop = getInputBool(kInput_Loop, true);
	const bool autoPlay = getInputBool(kInput_AutoPlay, true);
	const float volume = getInputFloat(kInput_Volume, 1.f);
	
	if (autoPlay)
	{
		audioStream = new AudioStream_Vorbis();
		audioStream->Open(source, loop);
		
		mixingAudioStream.timeInSamples = 0;
		
		isPaused = false;
		
		audioOutput = new AudioOutput_PortAudio();
		audioOutput->Initialize(2, 44100, 256);
		audioOutput->Volume_set(volume);
		audioOutput->Play(&mixingAudioStream);
		
		Assert(timeOutput == 0.f);
		Assert(beatCountOutput == 0);
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
			
			SDL_LockMutex(mutex);
			{
				audioStream->Open(source, loop);
				
				mixingAudioStream.timeInSamples = 0;
			}
			SDL_UnlockMutex(mutex);
			
			timeOutput = 0.f;
			
			beatCountOutput = 0;
		}
		
		isPaused = false;
		
		trigger(kOutput_Play);
	}
	else if (inputSocketIndex == kInput_Stop)
	{
		SDL_LockMutex(mutex);
		{
			audioStream->Close();
			
			mixingAudioStream.timeInSamples = 0;
		}
		SDL_UnlockMutex(mutex);
		
		timeOutput = 0.f;
		
		beatCountOutput = 0;
	}
	else if (inputSocketIndex == kInput_Restart)
	{
		const char * source = getInputString(kInput_Source, "");
		const bool loop = getInputBool(kInput_Loop, true);
		
		SDL_LockMutex(mutex);
		{
			audioStream->Open(source, loop);
			
			mixingAudioStream.timeInSamples = 0;
		}
		SDL_UnlockMutex(mutex);
		
		isPaused = false;
		
		timeOutput = 0.f;
		
		beatCountOutput = 0;
		
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
