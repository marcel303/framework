#include <AudioToolbox/AudioToolbox.h>

#if defined(IPHONEOS)
	#define kOutputBus 0
	#define kInputBus 1
#else
	#define kOutputBus 0
	#define kInputBus 1
#endif

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
	// todo : ios : AudioSessionSetProperty(kAudioSessionProperty_PreferredHardwareIOBufferDuration, .005f); // 5ms
	
	AudioComponent inputComponent = nullptr;
	
	{
		AudioComponentDescription desc;
		memset(&desc, 0, sizeof(desc));
		desc.componentType = kAudioUnitType_Output;
	#if defined(IPHONEOS)
		desc.componentSubType = kAudioUnitSubType_RemoteIO;
	#else
		desc.componentSubType = kAudioUnitSubType_DefaultOutput;
		//desc.componentSubType = kAudioUnitSubType_HALOutput;
	#endif
		desc.componentFlags = 0;
		desc.componentFlagsMask = 0;
		desc.componentManufacturer = kAudioUnitManufacturer_Apple;
		
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
	}

#if defined(IPHONEOS)
	{
		UInt32 flag = 1;
		
		OSStatus status;
		
		status = AudioUnitSetProperty(
			audioUnit,
			kAudioOutputUnitProperty_EnableIO,
			kAudioUnitScope_Input,
			kInputBus,
			&flag, sizeof(flag));
		checkStatus(status);
		
		status = AudioUnitSetProperty(
			audioUnit,
			kAudioOutputUnitProperty_EnableIO,
			kAudioUnitScope_Output,
			kOutputBus,
			&flag, sizeof(flag));
		checkStatus(status);
	}
#endif

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
