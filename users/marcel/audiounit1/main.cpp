#include <AudioToolbox/AudioToolbox.h>

// from : https://developer.apple.com/library/archive/documentation/MusicAudio/Conceptual/AudioUnitHostingGuide_iOS/AudioUnitHostingFundamentals/AudioUnitHostingFundamentals.html#//apple_ref/doc/uid/TP40009492-CH3-SW11,
// --> If setting a property or parameter that applies to a scope as a whole, specify an element value of 0.

#define kOutputBus 0
#define kInputBus 1

/*

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

static void checkStatus(OSStatus status)
{
	if (status != noErr)
	{
		printf("error: %d | %x\n", (int)status, (int)status);
	}
}

static OSStatus outputCallback(
	void *							inRefCon,
	AudioUnitRenderActionFlags *	ioActionFlags,
	const AudioTimeStamp *			inTimeStamp,
	UInt32							inBusNumber,
	UInt32							inNumberFrames,
	AudioBufferList * __nullable	ioData)
{
	for (int i = 0; i < ioData->mNumberBuffers; ++i)
	{
		auto & buffer = ioData->mBuffers[i];
		
		memset(buffer.mData, 0, buffer.mDataByteSize);
		
		// hack : synthesize a 440 Hz sine tone
		
		static double time = 0.0;
		static const int kSampleRate = 44100;

		float * samples = (float*)buffer.mData;
		const int numSamples = buffer.mDataByteSize / sizeof(float);
		
		for (int i = 0; i < numSamples; ++i)
		{
			//samples[i] = sinf(time);
			samples[i] = sinf(time * 440.0 * 2.0 * M_PI);
			
			time += 1.0 / kSampleRate;
			time = fmod(time, 1.0);
		}
	}
	
	return noErr;
}

int main(int argc, char * argv[])
{
	AudioComponent inputComponent = nullptr;
	
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
		
		inputComponent = AudioComponentFindNext(nullptr, &desc);
	}
	
	AudioComponentInstance audioUnit = nullptr;
	
	{
		auto status = AudioComponentInstanceNew(inputComponent, &audioUnit);
		checkStatus(status);
	}
	
	{
		AudioStreamBasicDescription sdesc;
		memset(&sdesc, 0, sizeof(sdesc));
		sdesc.mSampleRate = 44100;
		sdesc.mFormatID = kAudioFormatLinearPCM;
		sdesc.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
		sdesc.mFramesPerPacket = 1;
		sdesc.mChannelsPerFrame = 1;
		sdesc.mBitsPerChannel = 32;
		sdesc.mBytesPerFrame = 4;
		sdesc.mBytesPerPacket = 4;
		
		OSStatus status;
		UInt32 size;
		Boolean writable;
		
		status = AudioUnitGetPropertyInfo(
			audioUnit,
			kAudioUnitProperty_StreamFormat,
			kAudioUnitScope_Output,
			kInputBus,
			&size,
			&writable);
		checkStatus(status);
		
		printf("stream format is writable: %d\n", writable);
	
		if (writable)
		{
		#if 1
			status = AudioUnitSetProperty(
				audioUnit,
				kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Output,
				kInputBus,
				&sdesc, sizeof(sdesc));
			checkStatus(status);
		#endif
		}
		
	#if 1
		status = AudioUnitGetPropertyInfo(
			audioUnit,
			kAudioUnitProperty_StreamFormat,
			kAudioUnitScope_Input,
			kOutputBus,
			&size,
			&writable);
		checkStatus(status);
		
		printf("stream format is writable: %d\n", writable);
		
		if (writable)
		{
		#if 1
			status = AudioUnitSetProperty(
				audioUnit,
				kAudioUnitProperty_StreamFormat,
				kAudioUnitScope_Input,
				kOutputBus,
				&sdesc, sizeof(sdesc));
			checkStatus(status);
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
		checkStatus(status);
	#endif
		
	#if 0
		status = AudioUnitSetProperty(
			audioUnit,
			kAudioOutputUnitProperty_EnableIO,
			kAudioUnitScope_Output,
			kOutputBus,
			&flag, sizeof(flag));
		checkStatus(status);
	#endif
	}
#endif

	{
		// set maximum frame count
		
		UInt32 maxFramesPerSlice = 256;
		
		auto status = AudioUnitSetProperty(
			audioUnit,
			kAudioUnitProperty_MaximumFramesPerSlice,
			kAudioUnitScope_Global,
			0,
			&maxFramesPerSlice, sizeof(maxFramesPerSlice));
		checkStatus(status);
	}

	OSStatus status;
	
	// set output callback
	
	AURenderCallbackStruct cbs;
	cbs.inputProc = outputCallback;
	cbs.inputProcRefCon = (void*)0x1234; // user data
	status = AudioUnitSetProperty(
		audioUnit,
		kAudioUnitProperty_SetRenderCallback,
		kAudioUnitScope_Output,
		kOutputBus,
		&cbs, sizeof(cbs));
	checkStatus(status);
	
	status = AudioUnitInitialize(audioUnit);
	checkStatus(status);
	
	status = AudioOutputUnitStart(audioUnit);
	checkStatus(status);
	
	sleep(1000000);
	
	status = AudioOutputUnitStop(audioUnit);
	checkStatus(status);
	
	status = AudioComponentInstanceDispose(audioUnit);
	checkStatus(status);
	
	return 0;
}
