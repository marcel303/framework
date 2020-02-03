/*
	Copyright (C) 2020 Marcel Smit
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

#if FRAMEWORK_USE_COREAUDIO

#include "AudioOutput_CoreAudio.h"
#include "framework.h"

/*

from : https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitHostingGuide_iOS/AudioUnitHostingFundamentals/AudioUnitHostingFundamentals.html#//apple_ref/doc/uid/TP40009492-CH3-SW11,
	If setting a property or parameter that applies to a scope as a whole, specify an element value of 0.

--

	kAudioOutputUnitProperty_EnableIO,
		for enabling or disabling input or output on an I/O unit. By default, output is enabled but input is disabled.

	kAudioUnitProperty_MaximumFramesPerSlice,
		for specifying the maximum number of frames of audio data an audio unit should be prepared to produce in response to a render call.
 
 	kAudioUnitProperty_StreamFormat,
 		for specifying the audio stream data format for a particular audio unit input or output bus.

--

An I/O unit’s bus 0 connects to output hardware, such as for playback through a speaker. Output is enabled by default. To disable output, the bus 0 output scope must be disabled, as follows:

An I/O unit’s bus 1 connects to input hardware, such as for recording from a microphone. Input is disabled by default. To enable input, the bus 1 input scope must be enabled, as follows:

(AudioUnitSetProperty, kAudioOutputUnitProperty_EnableIO)

--

Frame count	Milliseconds at 44.1 kHz (approximate)
	Default	1024	23
	Screen sleep	4096	93
	Low latency	256	5


	You never need to set this property for I/O units because they are preconfigured to handle any slice size requested by the system. For all other audio units, you must set this property to a value of 4096 to handle screen sleep—unless audio input is running on the device. When audio input is running, the system maintains a slice size of 1024.

	--> lower audio latency is maintained when input is enabled
*/

#define kOutputBus 0
#define kInputBus 1

static bool checkStatus(OSStatus status)
{
	if (status != noErr)
	{
		logError("error: %d | %x", (int)status, (int)status);
		return false;
	}
	else
	{
		return true;
	}
}

void AudioOutput_CoreAudio::lock()
{
	Verify(SDL_LockMutex(m_mutex) == 0);
}

void AudioOutput_CoreAudio::unlock()
{
	Verify(SDL_UnlockMutex(m_mutex) == 0);
}

bool AudioOutput_CoreAudio::initCoreAudio(const int numChannels, const int sampleRate, const int bufferSize)
{
	{
		AudioComponentDescription desc;
		memset(&desc, 0, sizeof(desc));
		
		desc.componentType = kAudioUnitType_Output;
	#if defined(IPHONEOS)
		desc.componentSubType = kAudioUnitSubType_RemoteIO;
	#else
		desc.componentSubType = kAudioUnitSubType_DefaultOutput;
	#endif
		desc.componentManufacturer = kAudioUnitManufacturer_Apple;
		desc.componentFlags = 0;
		desc.componentFlagsMask = 0;
		
		m_audioComponent = AudioComponentFindNext(nullptr, &desc);
		
		if (m_audioComponent == nullptr)
			return false;
	}
	
	{
		auto status = AudioComponentInstanceNew(m_audioComponent, &m_audioUnit);
		if (checkStatus(status) == false)
			return false;
	}

	{
		AudioStreamBasicDescription sdesc;
		memset(&sdesc, 0, sizeof(sdesc));
		
		sdesc.mSampleRate = sampleRate;
		sdesc.mFormatID = kAudioFormatLinearPCM;
		sdesc.mFormatFlags =
			kAudioFormatFlagIsSignedInteger |
			kAudioFormatFlagIsPacked |
			kAudioFormatFlagsNativeEndian;
		sdesc.mFramesPerPacket = 1;
		sdesc.mChannelsPerFrame = numChannels;
		sdesc.mBitsPerChannel = 16;
		sdesc.mBytesPerFrame = 2 * numChannels;
		sdesc.mBytesPerPacket = 2 * numChannels;
		
		OSStatus status;
		UInt32 size;
		Boolean writable;
		
		status = AudioUnitGetPropertyInfo(
			m_audioUnit,
			kAudioUnitProperty_StreamFormat,
			kAudioUnitScope_Output,
			kInputBus,
			&size,
			&writable);
		if (checkStatus(status) == false)
			return false;
		
		logDebug("stream format is writable: %d", writable);
	
		if (writable)
		{
		#if 1
			status = AudioUnitSetProperty(
				m_audioUnit,
				kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Output,
				kInputBus,
				&sdesc, sizeof(sdesc));
			if (checkStatus(status) == false)
				return false;
		#endif
		}
		
	#if 1
		status = AudioUnitGetPropertyInfo(
			m_audioUnit,
			kAudioUnitProperty_StreamFormat,
			kAudioUnitScope_Input,
			kOutputBus,
			&size,
			&writable);
		if (checkStatus(status) == false)
			return false;
		
		logDebug("stream format is writable: %d", writable);
		
		if (writable)
		{
		#if 1
			status = AudioUnitSetProperty(
				m_audioUnit,
				kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Input,
				kOutputBus,
				&sdesc, sizeof(sdesc));
			if (checkStatus(status) == false)
				return false;
		#endif
		}
	#endif
	}

#if defined(IPHONEOS)
	{
		// verified correctness : https://developer.apple.com/library/archive/samplecode/aurioTouch/Listings/Classes_AudioController_mm.html#//apple_ref/doc/uid/DTS40007770-Classes_AudioController_mm-DontLinkElementID_4
		
		UInt32 flag = 1;
		
		OSStatus status;
		
	#if 0
		status = AudioUnitSetProperty(
			audioUnit,
			kAudioOutputUnitProperty_EnableIO,
			kAudioUnitScope_Input,
			kInputBus,
			&flag, sizeof(flag));
		if (checkStatus(status) == false)
			return false;
	#endif
		
	#if 0
		status = AudioUnitSetProperty(
			audioUnit,
			kAudioOutputUnitProperty_EnableIO,
			kAudioUnitScope_Output,
			kOutputBus,
			&flag, sizeof(flag));
		if (checkStatus(status) == false)
			return false;
	#endif
	}
#endif

	{
		// set maximum frame count
		
		UInt32 maxFramesPerSlice = bufferSize;
		
		auto status = AudioUnitSetProperty(
			m_audioUnit,
			kAudioUnitProperty_MaximumFramesPerSlice,
			kAudioUnitScope_Global,
			0,
			&maxFramesPerSlice, sizeof(maxFramesPerSlice));
		if (checkStatus(status) == false)
			return false;
	}

	OSStatus status;
	
	// set output callback
	
	AURenderCallbackStruct cbs;
	cbs.inputProc = outputCallback;
	cbs.inputProcRefCon = this;
	status = AudioUnitSetProperty(
		m_audioUnit,
		kAudioUnitProperty_SetRenderCallback,
		kAudioUnitScope_Output,
		kOutputBus,
		&cbs, sizeof(cbs));
	if (checkStatus(status) == false)
			return false;
	
	status = AudioUnitInitialize(m_audioUnit);
	if (checkStatus(status) == false)
		return false;
	
	status = AudioOutputUnitStart(m_audioUnit);
	if (checkStatus(status) == false)
		return false;

	m_numChannels = numChannels;
	m_sampleRate = sampleRate;

	return true;
}

bool AudioOutput_CoreAudio::shutCoreAudio()
{
	bool result = true;
	
	if (m_audioUnit != nullptr)
	{
		auto status = AudioOutputUnitStop(m_audioUnit);
		result &= checkStatus(status);
		
		status = AudioComponentInstanceDispose(m_audioUnit);
		result &= checkStatus(status);
		
		m_audioUnit = nullptr;
	}
	
	m_numChannels = 0;
	m_sampleRate = 0;

	return result;
}

OSStatus AudioOutput_CoreAudio::outputCallback(
	void * inRefCon,
	AudioUnitRenderActionFlags * ioActionFlags,
	const AudioTimeStamp * inTimeStamp,
	UInt32 inBusNumber,
	UInt32 inNumberFrames,
	AudioBufferList * __nullable ioData)
{
	AudioOutput_CoreAudio * self = (AudioOutput_CoreAudio*)inRefCon;
	
	Assert(ioData->mNumberBuffers == 1);
	
	for (int i = 0; i < ioData->mNumberBuffers && i < 1; ++i)
	{
		auto & buffer = ioData->mBuffers[i];
		
		AudioSample * __restrict samples;
		const int numSamples = inNumberFrames;

		if (self->m_numChannels == 2)
			samples = (AudioSample*)buffer.mData;
		else
			samples = (AudioSample*)alloca(numSamples * sizeof(AudioSample));
		
		bool generateSilence = true;
		
		self->lock();
		{
			if (self->m_stream && self->m_isPlaying)
			{
				generateSilence = false;
				
				const int numSamplesRead = self->m_stream->Provide(numSamples, samples);
				
				memset(samples + numSamplesRead, 0, (numSamples - numSamplesRead) * sizeof(int16_t) * self->m_numChannels);
				
				self->m_position += numSamplesRead;
				self->m_isDone = numSamplesRead == 0;
			}
		}
		self->unlock();
		
		if (generateSilence)
		{
			Assert(numSamples * sizeof(int16_t) * self->m_numChannels == buffer.mDataByteSize);
			memset(buffer.mData, 0, numSamples * sizeof(int16_t) * self->m_numChannels);
		}
		else if (self->m_numChannels == 1)
		{
			const int volume = std::max(0, std::min(1024, self->m_volume.load()));

			int16_t * __restrict values = (int16_t*)buffer.mData;
		
			for (int i = 0; i < numSamples; ++i)
				values[i] = (int(samples[i].channel[0] + samples[i].channel[1]) * volume) >> 11;
		}
		else
		{
			Assert(self->m_numChannels == 2);
			Assert(samples == buffer.mData);
			
			const int volume = std::max(0, std::min(1024, self->m_volume.load()));

			if (volume != 1024)
			{
				int16_t * __restrict values = (int16_t*)buffer.mData;
				const int numValues = numSamples * 2;
			
				for (int i = 0; i < numValues; ++i)
				{
					values[i] = (int(values[i]) * volume) >> 10;
				}
			}
		}
	}
	
	return noErr;
}

AudioOutput_CoreAudio::AudioOutput_CoreAudio()
	: m_isPlaying(false)
	, m_volume(1024)
	, m_position(0)
	, m_isDone(false)
{
	m_mutex = SDL_CreateMutex();
	Assert(m_mutex != nullptr);
}

AudioOutput_CoreAudio::~AudioOutput_CoreAudio()
{
	Shutdown();
	
	Assert(m_mutex != nullptr);
	SDL_DestroyMutex(m_mutex);
	m_mutex = nullptr;
}

bool AudioOutput_CoreAudio::Initialize(const int numChannels, const int sampleRate, const int bufferSize)
{
	fassert(numChannels == 1 || numChannels == 2);

	if (numChannels != 1 && numChannels != 2)
	{
		logError("portaudio: invalid number of channels");
		return false;
	}

	if (initCoreAudio(numChannels, sampleRate, bufferSize) == false)
	{
		shutCoreAudio();
		return false;
	}
	else
	{
		return true;
	}
}

bool AudioOutput_CoreAudio::Shutdown()
{
	Stop();
	
	return shutCoreAudio();
}

void AudioOutput_CoreAudio::Play(AudioStream * stream)
{
	lock();
	{
		m_isPlaying = true;
		m_isDone = false;
		
		m_stream = stream;
	}
	unlock();
}

void AudioOutput_CoreAudio::Stop()
{
	lock();
	{
		m_isPlaying = false;
		
		m_stream = nullptr;
		
		m_position = 0;
	}
	unlock();
}

void AudioOutput_CoreAudio::Update()
{
	// nothing to do here
}

void AudioOutput_CoreAudio::Volume_set(float volume)
{
	Assert(volume >= 0.f && volume <= 1.f);

	m_volume = int(roundf(volume * 1024.f));
}

bool AudioOutput_CoreAudio::IsPlaying_get()
{
	return m_isPlaying;
}

bool AudioOutput_CoreAudio::HasFinished_get()
{
	return m_isDone;
}

double AudioOutput_CoreAudio::PlaybackPosition_get()
{
	return m_position / double(m_sampleRate);
}

#endif
