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

#if AUDIOOUTPUT_HD_USE_COREAUDIO

#include "AudioOutputHD_CoreAudio.h"

#include "Debugging.h"
#include "Log.h"

#if defined(IPHONEOS)
	#include "AVFoundation/AVAudioSession.h"
#endif

#include <mach/mach_time.h> // mach_timebase_info

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
		LOG_ERR("error: %d | %x", (int)status, (int)status);
		return false;
	}
	else
	{
		return true;
	}
}

bool AudioOutputHD_CoreAudio::initCoreAudio(const int numInputChannels, const int numOutputChannels, const int frameRate, const int bufferSize)
{
	Assert(m_numInputChannels == 0);
	
	{
		// create the output/remoteio audio component
		
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
		
		auto status = AudioComponentInstanceNew(m_audioComponent, &m_audioUnit);
		if (checkStatus(status) == false)
			return false;
	}

	{
		// set the stream format on the output component
		
		AudioStreamBasicDescription sdesc;
		memset(&sdesc, 0, sizeof(sdesc));
		
	#if 1
		sdesc.mSampleRate = frameRate;
		sdesc.mFormatID = kAudioFormatLinearPCM;
		sdesc.mFormatFlags =
			kAudioFormatFlagIsFloat |
			kAudioFormatFlagIsNonInterleaved;
			//kAudioFormatFlagIsPacked |
			//kAudioFormatFlagsNativeEndian;
		sdesc.mFramesPerPacket = 1; // In uncompressed audio, a Packet is one frame, (mFramesPerPacket == 1). In compressed audio, a Packet is an indivisible chunk of compressed data, for example an AAC packet will contain 1024 sample frames.
		sdesc.mChannelsPerFrame = numOutputChannels;
		sdesc.mBitsPerChannel = sizeof(float) * 8;
		sdesc.mBytesPerFrame = sizeof(float);
		sdesc.mBytesPerPacket = sizeof(float);
	#else
		sdesc.mSampleRate = frameRate;
		sdesc.mFormatID = kAudioFormatLinearPCM;
		sdesc.mFormatFlags =
			kAudioFormatFlagIsFloat |
			kAudioFormatFlagIsPacked |
			kAudioFormatFlagsNativeEndian;
		sdesc.mFramesPerPacket = 1; // In uncompressed audio, a Packet is one frame, (mFramesPerPacket == 1). In compressed audio, a Packet is an indivisible chunk of compressed data, for example an AAC packet will contain 1024 sample frames.
		sdesc.mChannelsPerFrame = numOutputChannels;
		sdesc.mBitsPerChannel = sizeof(float) * 8;
		sdesc.mBytesPerFrame = numOutputChannels * sizeof(float);
		sdesc.mBytesPerPacket = numOutputChannels * sizeof(float);
	#endif
    
		OSStatus status;
		UInt32 size;
		Boolean writable;
		
	#if defined(IPHONEOS)
		status = AudioUnitGetPropertyInfo(
			m_audioUnit,
			kAudioUnitProperty_StreamFormat,
			kAudioUnitScope_Output,
			kInputBus,
			&size,
			&writable);
		if (checkStatus(status) == false)
			return false;
		
		//LOG_DBG("stream format is writable: %d", writable);
	
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
	#endif
		
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
		
		//LOG_DBG("stream format is writable: %d", writable);
		
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
		// set input/output enables
		
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
	
		UInt32 maxFramesPerSlice = 4096; // macOS prefers to use a large buffer size (4096) when in power saving mode
		
		auto status = AudioUnitSetProperty(
			m_audioUnit,
			kAudioUnitProperty_MaximumFramesPerSlice,
			kAudioUnitScope_Global,
			0,
			&maxFramesPerSlice, sizeof(maxFramesPerSlice));
		if (checkStatus(status) == false)
			return false;
	}
	
#if !defined(IPHONEOS)
	{
		// set the buffer size
		
		const UInt32 bufferSize_u32 = bufferSize; // todo : use specified buffer size or try multiples thereof. make sure during Provide to use the specified buffer size!
	
		auto status = AudioUnitSetProperty(
			m_audioUnit,
			kAudioDevicePropertyBufferFrameSize,
			kAudioUnitScope_Output,
			0,
			&bufferSize_u32, sizeof(bufferSize_u32));
		checkStatus(status);
	}
	
	{
		// check the buffer size
		
		UInt32 value = 0;
		UInt32 value_size = sizeof(value);
		
		auto status = AudioUnitGetProperty(
			m_audioUnit,
			kAudioDevicePropertyBufferFrameSize,
			kAudioUnitScope_Output,
			0,
			&value,
			&value_size);
		checkStatus(status);
		
		LOG_DBG("got buffer size: %d", (int)value);
	}
#endif

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
	
	// fill in stream info
	
	m_streamInfo.frameRate = frameRate;
	m_streamInfo.secondsSincePlay = 0;
	m_streamInfo.framesSincePlay = 0;
	m_streamInfo.outputLatency = 0.f;
	
	m_numInputChannels = numInputChannels;
	m_numOutputChannels = numOutputChannels;
	m_frameRate = frameRate;
	m_bufferSize = bufferSize;

#if defined(IPHONEOS)
	// set the preferred output buffer duration
	
	[[AVAudioSession sharedInstance] setPreferredIOBufferDuration:bufferSize / double(frameRate) error:nil];
	
	// capture the current output latency
	
	m_streamInfo.outputLatency = [AVAudioSession sharedInstance].outputLatency;
	
	// respond to AVAudioSession changes to update the captured output latency
	
	[[NSNotificationCenter defaultCenter]
		addObserverForName:AVAudioSessionRouteChangeNotification
		object:nil
		queue:nil
		usingBlock:^(NSNotification * note)
		{
			NSLog(@"detected AVAudioSession route change");
			
			m_streamInfo.outputLatency = [AVAudioSession sharedInstance].outputLatency;
		}];
#else
	m_streamInfo.outputLatency = 0.f;
#endif

	return true;
}

bool AudioOutputHD_CoreAudio::shutCoreAudio()
{
	bool result = true;
	
	if (m_audioUnit != nullptr)
	{
		auto status = AudioComponentInstanceDispose(m_audioUnit);
		result &= checkStatus(status);
		
		m_audioUnit = nullptr;
	}
	
	m_numInputChannels = 0;
	m_numOutputChannels = 0;
	m_frameRate = 0;
	m_bufferSize = 0;
	
	m_streamInfo = AudioStreamHD::StreamInfo();

	return result;
}

OSStatus AudioOutputHD_CoreAudio::outputCallback(
	void * inRefCon,
	AudioUnitRenderActionFlags * ioActionFlags,
	const AudioTimeStamp * inTimeStamp,
	UInt32 inBusNumber,
	UInt32 inNumberFrames,
	AudioBufferList * __nullable ioData)
{
	AudioOutputHD_CoreAudio * self = (AudioOutputHD_CoreAudio*)inRefCon;
	
	Assert(ioData->mNumberBuffers == self->m_numOutputChannels);
	
	self->m_bufferPresentTime = inTimeStamp->mHostTime;

	AudioStreamHD::ProvideInfo provideInfo;
	provideInfo.inputSamples = nullptr;
	provideInfo.numInputChannels = 0;
	provideInfo.outputSamples = (float**)alloca(self->m_numOutputChannels * sizeof(float*));
	provideInfo.numOutputChannels = self->m_numOutputChannels;
	provideInfo.numFrames = inNumberFrames;
	
	auto & streamInfo = self->m_streamInfo;
	
	for (int i = 0; i < self->m_numOutputChannels; ++i)
	{
		auto & buffer = ioData->mBuffers[i];
		
		Assert(buffer.mNumberChannels == 1);
		Assert(buffer.mDataByteSize == inNumberFrames * sizeof(float));
		
		float * outputSamples = (float*)buffer.mData;
		
		provideInfo.outputSamples[i] = outputSamples;
	}
	
	int numChannelsProvided = 0;
	
	self->m_mutex.lock();
	{
		if (self->m_stream && self->m_isPlaying)
		{
			streamInfo.framesSincePlay = self->m_framesSincePlay.load();
			streamInfo.secondsSincePlay = streamInfo.framesSincePlay / double(self->m_frameRate);
			
			numChannelsProvided = self->m_stream->Provide(provideInfo, streamInfo);
			
			self->m_framesSincePlay.store(self->m_framesSincePlay.load() + inNumberFrames);
			
		#if defined(DEBUG) && 0
			for (int i = 0; i < numChannelsProvided; ++i)
			{
				for (int s = 0; s < provideInfo.numFrames; ++s)
				{
					Assert(
						provideInfo.outputSamples[i][s] >= -1.f &&
						provideInfo.outputSamples[i][s] <= +1.f);
				}
			}
		#endif
		}
	}
	self->m_mutex.unlock();
	
	// mute channels that weren't provided
	
	for (int i = numChannelsProvided; i < provideInfo.numOutputChannels; ++i)
	{
		memset(provideInfo.outputSamples[i], 0, provideInfo.numFrames * sizeof(float));
	}
	
	// apply volume
	
	const float volume = self->m_volume.load();
	
	Assert(volume >= 0.f && volume <= 1.f);
	
	for (int i = 0; i < self->m_numOutputChannels; ++i)
	{
		for (int s = 0; s < provideInfo.numFrames; ++s)
		{
			provideInfo.outputSamples[i][s] *= volume;
		}
	}
	
	return noErr;
}

AudioOutputHD_CoreAudio::AudioOutputHD_CoreAudio()
	: m_isPlaying(false)
	, m_volume(1.f)
	, m_framesSincePlay(0)
{
}

AudioOutputHD_CoreAudio::~AudioOutputHD_CoreAudio()
{
	Shutdown();
}

bool AudioOutputHD_CoreAudio::Initialize(const int numInputChannels, const int numOutputChannels, const int frameRate, const int bufferSize)
{
	if (initCoreAudio(numInputChannels, numOutputChannels, frameRate, bufferSize) == false)
	{
		shutCoreAudio();
		return false;
	}
	else
	{
		return true;
	}
}

bool AudioOutputHD_CoreAudio::Shutdown()
{
	Stop();
	
	return shutCoreAudio();
}

void AudioOutputHD_CoreAudio::Play(AudioStreamHD * stream)
{
	Assert(m_framesSincePlay.load() == 0);
	
	m_mutex.lock();
	{
		m_isPlaying = true;
		
		m_stream = stream;
	}
	m_mutex.unlock();
	
	if (m_audioUnit != nullptr)
	{
		if (checkStatus(AudioOutputUnitStart(m_audioUnit)) == false)
			LOG_ERR("failed to start audio unit");
	}
}

void AudioOutputHD_CoreAudio::Stop()
{
	if (m_audioUnit != nullptr)
	{
		if (checkStatus(AudioOutputUnitStop(m_audioUnit)) == false)
			LOG_ERR("failed to stop audio unit");
	}
		
	m_mutex.lock();
	{
		m_stream = nullptr;
		
		m_isPlaying = false;
		
		m_framesSincePlay = 0;

	}
	m_mutex.unlock();
}

void AudioOutputHD_CoreAudio::Volume_set(const float volume)
{
	Assert(volume >= 0.f && volume <= 1.f);

	m_volume.store(volume);
}

float AudioOutputHD_CoreAudio::Volume_get() const
{
	return m_volume.load();
}

bool AudioOutputHD_CoreAudio::IsPlaying_get() const
{
	return m_isPlaying.load();
}

int AudioOutputHD_CoreAudio::BufferSize_get() const
{
	return m_bufferSize;
}

int AudioOutputHD_CoreAudio::FrameRate_get() const
{
	return m_frameRate;
}

uint64_t AudioOutputHD_CoreAudio::getBufferPresentTimeUs(const bool addOutputLatency) const
{
	uint64_t result = m_bufferPresentTime;
	
	// convert form ticks to microseconds
	mach_timebase_info_data_t timeInfo;
	mach_timebase_info(&timeInfo);
	result = uint64_t(result * double(timeInfo.numer / (timeInfo.denom * 1000.0)));
	
#if defined(IPHONEOS)
	if (addOutputLatency)
		result += uint64_t(m_streamInfo.outputLatency * 1e6);
#endif

	return result;
}

#endif
