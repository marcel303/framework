#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#import "MainController.h"
#include "Synth.h"

@implementation MainController

static ALCdevice* g_alDevice = 0;
static ALCcontext* g_alContext = 0;
static bool g_alInitialized = false;

/*
 * Test playback of one of the built-in system sounds.
 */
-(IBAction)touchPlaySoundSound:(id)sender
{
	// Play vibration system 'sound'.
	
	AudioServicesPlayAlertSound(kSystemSoundID_Vibrate);
	
	//
	
	[lblStatus setText:@"Play: Vibration"];
}

/*
 * Test playback of a custom system sound.
 */
-(IBAction)touchPlaySoundSoundCustom:(id)sender
{
	// Randomly select sound sample.
	
	NSString* files[] = { @"sound_voices", @"click_off", @"sound1" };
	
	int fileIndex = rand() % 3;
	
	// Create path to resource file.
	
	NSString* path = [[NSBundle mainBundle] pathForResource:files[fileIndex] ofType:@"wav"];
	
	NSURL* url = [NSURL fileURLWithPath:path];
	
	// Create system sound.
	// todo: Should the system sound be created once or does the framework cache these for us?
	
	SystemSoundID soundId;
	
	AudioServicesCreateSystemSoundID((CFURLRef)url, &soundId);
	
	// Play system sound.
	
	AudioServicesPlayAlertSound(soundId);
	
	//
	
	[lblStatus setText:[@"Play: " stringByAppendingString:files[fileIndex]]];
}

-(IBAction)touchPlayStreamHW:(id)sender
{
	[self testAudioPlayer];
	
	[lblStatus setText:@"Play: StreamHW"];
}

static void SWCallback_Interrupt(void* p, UInt32 c)
{
}

// AVAudioPlayer stuff.

-(void)testAudioPlayer
{
	[m_AudioPlayer release];
	
	// Create audio player.
	
//	NSString* path = [[NSBundle mainBundle] pathForResource:@"sound_voices" ofType:@"wav"];
	NSString* path = [[NSBundle mainBundle] pathForResource:@"sfx_volumetest" ofType:@"wav"];
	
	NSURL* url = [[NSURL alloc] initFileURLWithPath:path];
	
	m_AudioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:url error: nil];
	
	[url release];
	
	// If we want to make use of callbacks, set the audio player's delegate to self.
	
	//	[m_AudioPlayer setDelegate: self];
	
	// Set volume.
	
	[m_AudioPlayer setVolume:0.5f];
	
	// Play.
	
	[m_AudioPlayer play];
}

// Audio Queue stuff.

static const int bufferCount = 3;

// Wrapper

enum AudioSampleFormat
{
	AudioSampleFormat_Int16,
	AudioSampleFormat_Int32,
	AudioSampleFormat_Float32
};

class AudioStreamDescription
{
public:
	AudioStreamDescription()
	{
	}
	
	AudioStreamDescription(
		int sampleRate,
		AudioSampleFormat format,
		int samplesPerFrame,
		int channelsPerFrame)
	{
		m_SamplesPerFrame = samplesPerFrame;
		m_ChannelsPerFrame = channelsPerFrame;
		
		UInt32 bytesPerSample = FormatToSampleSize(format);
		UInt32 framesPerPacket = samplesPerFrame;
		UInt32 bitsPerChannel = bytesPerSample * 8;
		UInt32 bytesPerFrame = channelsPerFrame * bytesPerSample;
		UInt32 bytesPerPacket = bytesPerFrame * samplesPerFrame;
		
		m_Description.mSampleRate = (Float64)sampleRate;
		m_Description.mFormatID = kAudioFormatLinearPCM;
		m_Description.mFormatFlags = FormatToAppleFormatFlags(format);
		m_Description.mBytesPerPacket = bytesPerPacket;
		m_Description.mFramesPerPacket = framesPerPacket;
		m_Description.mBytesPerFrame = bytesPerFrame;
		m_Description.mChannelsPerFrame = channelsPerFrame;
		m_Description.mBitsPerChannel = bitsPerChannel;
		m_Description.mReserved = 0;
		
		/*
		// formatId =
		kAudioFormatLinearPCM
		kAudioFormatAC3   
		kAudioFormatMPEG4AAC  
		kAudioFormatAppleIMA4   
		kAudioFormatMPEGLayer3 
		kAudioFormatMIDIStream      
		kAudioFormatAppleLossless
		*/
		
		/*
		// formatFlags =
		kLinearPCMFormatFlagIsFloat
		kLinearPCMFormatFlagIsBigEndian
		kLinearPCMFormatFlagIsSignedInteger
		kLinearPCMFormatFlagIsPacked
		kLinearPCMFormatFlagIsAlignedHigh
		kLinearPCMFormatFlagIsNonInterleaved
		kLinearPCMFormatFlagIsNonMixable
		kLinearPCMFormatFlagsSampleFractionShift
		kLinearPCMFormatFlagsSampleFractionMask
		kLinearPCMFormatFlagsAreAllClear
		*/
	}
	
	static UInt32 FormatToAppleFormatFlags(AudioSampleFormat format)
	{
		UInt32 result = 0;
		
		result |= kAudioFormatFlagIsPacked;
		result |= kAudioFormatFlagsNativeEndian;
		
		switch (format)
		{
				case AudioSampleFormat_Int16:
				case AudioSampleFormat_Int32:
				result |= kLinearPCMFormatFlagIsSignedInteger; 
					break;
				
			case AudioSampleFormat_Float32:
				result |= kLinearPCMFormatFlagIsFloat;
				break;
		}
		
		return result;
	}
	
	static UInt32 FormatToSampleSize(AudioSampleFormat format)
	{
		switch (format)
		{
			case AudioSampleFormat_Int16:
				return 2;
				
			case AudioSampleFormat_Int32:
				return 4;
				
			case AudioSampleFormat_Float32:
				return 4;
				
			default:
				return 0;
		}
	}
	
	AudioStreamBasicDescription m_Description;
	int m_SamplesPerFrame;
	int m_ChannelsPerFrame;
};

class AudioState
{
public:
	AudioState()
	{
		Initialize(NULL);
	}
	
	AudioState(AudioStreamDescription streamDescription)
	{
		Initialize(&streamDescription);
	}
	
	void Initialize(AudioStreamDescription* streamDescription)
	{
		if (streamDescription)
			mDataFormat = *streamDescription;
		mQueue = NULL;
		for (int i = 0; i < bufferCount; ++i)
			mBuffers[i] = NULL;
		mDone = false;
	}
	
	AudioStreamDescription          mDataFormat;
    AudioQueueRef                   mQueue;
    AudioQueueBufferRef             mBuffers[bufferCount];
    bool                            mDone;
};

// Callback

#define SAMPLE_RATE 44100
//#define SAMPLE_RATE 11025

static Synth* g_Synth = NULL;
static std::vector<SynthSource_Note*> g_Notes;
static int g_NoteW = 0;

static void Synth_Initialize()
{
	g_Synth = new Synth();
	
	// Create envelope.
	
	SynthEnv env;
#if 1
	env.m_Stages[0].Set(0.25, 0.0, 1.0);
	env.m_Stages[1].Set(0.5, 1.0, 0.5);
	env.m_Stages[2].Set(1.0, 0.5, 0.5);
	env.m_Stages[3].Set(1.0, 0.5, 0.0);
#else
	env.m_Stages[0].Set(0.1, 0.0, 1.0);
	env.m_Stages[1].Set(0.3, 1.0, 0.5);
	env.m_Stages[2].Set(0.3, 0.5, 0.5);
	env.m_Stages[3].Set(0.1, 0.5, 0.0);
#endif

	// Create notes.
	
	for (int i = 0; i < 10; ++i)
	{
		SynthSource_Note* note = new SynthSource_Note();
//		note->m_Source = new SynthSourceBase(SynthSourceType_Triangle, 100.0);
		note->m_Source = new SynthSourceBase(SynthSourceType_Square, 100.0);
		note->m_Note.m_Env = env;
		
		g_Synth->m_Mix->m_Sources.push_back(note);
		
		g_Notes.push_back(note);
	}
}

static void Synth_GetSamples(short* buffer, int count)
{
	double* temp = new double[count];
	
	g_Synth->m_Mix->Generate(temp, count, 1.0 / SAMPLE_RATE);
	
	for (int i = 0; i < count; ++i)
	{
		buffer[i] = temp[i] * 5000.0; // fixme.
	}
	
	delete[] temp;
}

static void Synth_PlayNote(double frequency)
{
	SynthSource_Note* note = g_Notes[g_NoteW];
	
	g_NoteW = (g_NoteW + 1) % g_Notes.size();
	
//	note->m_Source = new SynthSourceBase(SynthSourceType_Square, frequency);
	note->m_Source = new SynthSourceBase(SynthSourceType_Sine, frequency);
//	note->m_Source = new SynthSourceBase(SynthSourceType_Triangle, frequency);
//	note->m_Source = new SynthSourceBase(SynthSourceType_Saw, frequency);
	note->Play();
}

static void AudioQueueCallback_GetSamples(
	 void* userData,
	 AudioQueueRef audioQueue,
	 AudioQueueBufferRef audioBuffer)
{
    AudioState* audioState = (AudioState*)userData;

    if (audioState->mDone)
		return;
	
	// todo: make external.
	bool stop = false;
	
	if (!stop)
	{
		// fixme.. generate sine, use channel count, samples/channel/frame.
		
		int sampleCount = audioState->mDataFormat.m_SamplesPerFrame * audioState->mDataFormat.m_ChannelsPerFrame;
		
        audioBuffer->mAudioDataByteSize = sampleCount * 2;
		
		short temp[sampleCount];

		Synth_GetSamples(temp, sampleCount);
		
		memcpy(audioBuffer->mAudioData, temp, sampleCount * 2);
		
        AudioQueueEnqueueBuffer(
			audioQueue,
			audioBuffer,
			0,
			NULL);
	}
	else
	{
        AudioQueueStop(
			audioState->mQueue,
			false);
		
        audioState->mDone = true;
	}
}

-(void)testAudioStream
{
	OSStatus rtStatus;
	
	// Instantiate an audio queue object
	
	AudioStreamDescription streamDescription(
		SAMPLE_RATE,
		AudioSampleFormat_Int16,
		1024,
		1);
	
	AudioState* state = new AudioState(streamDescription);
	
	rtStatus = AudioQueueNewOutput(
		&state->mDataFormat.m_Description,
		AudioQueueCallback_GetSamples,
		state,
		CFRunLoopGetCurrent(),
		kCFRunLoopCommonModes,
		0,
		&state->mQueue);
	
	if (rtStatus)
		NSLog(@"Failed to create new AudioQueue output");
	else
		NSLog(@"Created new AudioQueue output");
	
	// Set volume.
	
	Float32 volume = 1.0f;
	
	rtStatus = AudioQueueSetParameter(
		state->mQueue,
		kAudioQueueParam_Volume,
		volume);
	
	if (rtStatus)
		NSLog(@"Failed to set AudioQueue volume");
	else
		NSLog(@"AudioQueue volume set");
	
	//
	
	Synth_Initialize();
	
	//
	
	for (int i = 0; i < bufferCount; ++i)
	{
		AudioQueueAllocateBuffer(state->mQueue, 2048, &state->mBuffers[i]);

		AudioQueueCallback_GetSamples(state, state->mQueue, state->mBuffers[i]);
	}
		
	AudioQueueStart(state->mQueue, NULL);
	
//	AudioQueueDispose(state->mQueue, true);
}

//

#if 1

// AudioSession pause & resume functions.
// Required to handle interruptions due to calls, alarm clocks, etc.

static void AudioSession_Pause()
{
	// Suspend OpenAL context.
	alcSuspendContext(g_alContext);
	
	// Disable OpenAL.
	alcMakeContextCurrent(NULL);

	// Disable audio session.
	AudioSessionSetActive(NO);
}

static void AudioSession_Resume()
{
	/*
	 UInt32 sessionCategory = kAudioSessionCategory_AmbientSound;
	 
	 rtStatus = AudioSessionSetProperty(
		 kAudioSessionProperty_AudioCategory,
		 sizeof (sessionCategory),
		 &sessionCategory);
	 */
	
	// Enable audio session.
	AudioSessionSetActive(YES);
	
	// Enable OpenAL.
	alcMakeContextCurrent(g_alContext);

	// Resume OpenAL context.
	alcProcessContext(g_alContext);
	
}

static void AudioSession_InterruptionCallback(
	void* userData,
	UInt32 interruptionState)
{
//	MainController* controller = (MainController*)userData;
	
	if (interruptionState == kAudioSessionBeginInterruption)
	{
//		[controller _audioSessionPause];
		
		AudioSession_Pause();
	}
	else if (interruptionState == kAudioSessionEndInterruption)
	{
//		[controller _audioSessionResume];
		
		AudioSession_Resume();
	}
}

static void AudioSession_Initialize()
{
	OSStatus rtStatus;
	
	void* userData = 0;

	rtStatus = AudioSessionInitialize(
		NULL,
		NULL,
		AudioSession_InterruptionCallback,
		userData);

	if (rtStatus)
		NSLog(@"Failed to initialize AudioSession");
	else
		NSLog(@"AudioSession initialized");
	
	//
	
//	UInt32 sessionCategory = kAudioSessionCategory_MediaPlayback; // Disables iPod music playback.
	UInt32 sessionCategory = kAudioSessionCategory_AmbientSound; // Allows iPod music playback to be mixed.

	rtStatus = AudioSessionSetProperty(
		kAudioSessionProperty_AudioCategory,
		sizeof (sessionCategory),
		&sessionCategory);
	
	if (rtStatus)
		NSLog(@"Failed to set AudioSession catagory");
	else
		NSLog(@"AudioSession catagory set");
	
	rtStatus = AudioSessionSetActive(true);
	
	if (rtStatus)
		NSLog(@"Failed to make AudioSession active");
	else
		NSLog(@"AudioSession made active");
}

#endif

//

-(IBAction)touchPlayStreamSW:(id)sender
{
	AudioSession_Initialize();
	
	[self testAudioStream];
	
	[lblStatus setText:@"Play: StreamSW"];
}

-(IBAction)touchPlayStreamSW_Note:(id)sender//:(UIEvent*)withEvent
{
	//NSSet* touches = [withEvent touchesForView:self.view];
	
	//UITouch* touch = [touches anyObject];
	
	//CGPoint point = [touch locationInView:self.view];
	
	if (g_Synth == 0)
	{
		UIAlertView* alert = [[UIAlertView alloc] initWithTitle:nil message:@"Please press StreamSW first" delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
		[alert show];
		[alert release];
		
		return;
	}
	
	static double offset = 0.0;
	
	offset = fmod(offset + 111, 1000);
	
	double frequency = offset + 200;
//	double frequency = 400 + (rand() % 300);
	
	Synth_PlayNote(frequency);
	
	[lblStatus setText:@"Play: StreamSW NOTE"];
}

static void al_CheckError()
{
	ALenum error = alGetError();
	
	if (error)
		NSLog(@"error: %d", (int)error);
}

static void al_Initialize()
{
	if (g_alInitialized)
		return;
	
	g_alDevice = alcOpenDevice(0);
	al_CheckError();
	
	g_alContext = alcCreateContext(g_alDevice, 0);
	al_CheckError();
	
	alcMakeContextCurrent(g_alContext);
	al_CheckError();
	
	alSpeedOfSound(344.0f);
	al_CheckError();
	
	g_alInitialized = true;
}

static void al_Shutdown()
{
	alcDestroyContext(g_alContext);
	g_alContext = 0;
	al_CheckError();
	
	alcCloseDevice(g_alDevice);
	g_alDevice = 0;
	al_CheckError();
}

-(IBAction)touchOpenAL:(id)sender
{
	// iPhone supported audio buffer formats: mono 8-bit, mono 16-bit, stereo 8-bit, and stereo 16-bit.
	// 'The iPhone OS implementation of OpenAL does not include the effect extensions found on the desktop.'
	// -> no Roger Beep, Distortion, Reverb, Obstruction, and Occlusion effects.
	// 'The OpenAL capture APIs, which you would use for OpenAL recording, are not available in this version of the OS.'

	// Optimization:
	// - Use the alBufferDataStatic API, found in the oalStaticBufferExtension.h header file, instead of the standard alBufferData function.
	//   This eliminates extra buffer copies by allowing your application to own the audio data memory used by the buffer objects.
	// - Use lower sampling rates when mixing many different sources.
	// - Obvious: Reduce the number of sources by using an efficient culling strategy.
	
	// Audio conversion:
	// - iPhone natively uses little endian.
	// - Sampling rate etc may need to be fixed at 22KHz or 44KHz.
	// - Conversion: /usr/bin/afconvert -f caff -d LEI16@44100 inputSoundFile.aiff outputSoundFile.caf
	
	// Sound sources:
	// - Doppler shift is supported:
	//   * Sounds lose pitch when moving away from the listener.
	//   * Sounds increase in pitch when moving towards listener.
	// - Attenuation is supported:
	//   * Sound volume decreases as source is further away from listener.
	// - Orientation is taken into account.
	//   * Left/right panning is applied to create a spatial impression.
	
	al_Initialize();
	
	ALfloat listenerPos[] = { 0.0f, 0.0f, 0.0f };
	ALfloat listenerVel[] = { 0.0f, 0.0f, 0.0f };
	ALfloat listenerDir[] = { 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f };
	
	ALfloat sourcePos[] = { 0.0f, 0.0f, 1.0f };
	ALfloat sourceVel[] = { 0.0f, 0.0f, 0.0f };
	
	ALuint bufferId = 0;
	ALuint sourceId = 0;
	
	// Set listener properties.
	
	alListenerfv(AL_POSITION, listenerPos);
	alListenerfv(AL_VELOCITY, listenerVel);
	alListenerfv(AL_ORIENTATION, listenerDir);
	al_CheckError();
	
	// Create buffer.
	
	alGenBuffers(1, &bufferId);
	al_CheckError();
	
	// Fill buffer.
	
	int sampleCount = 1024;
	
	ALenum bufferFormat = AL_FORMAT_MONO16;
	ALuint bufferSize = sampleCount * 2;
	ALuint bufferSampleRate = SAMPLE_RATE;

	short bufferData[sampleCount];
	
	for (int i = 0; i < sampleCount; ++i)
		bufferData[i] = rand();
	
	alBufferData(bufferId, bufferFormat, bufferData, bufferSize, bufferSampleRate);
	al_CheckError();
	
	// Create sound source.
	
	alGenSources(1, &sourceId);

	// Set sound source properties.
	
	alSourcefv(sourceId, AL_POSITION, sourcePos);
	alSourcefv(sourceId, AL_VELOCITY, sourceVel);
	alSourcef(sourceId, AL_PITCH, 1.0f);
	alSourcef(sourceId, AL_GAIN, 1.0f);
	al_CheckError();
	
	// Play sound source.
	// - Stop source.
	// - Assign new buffer.
	// - Set loop flag.
	// - Start source.
	
	alSourceStop(sourceId);
	al_CheckError();

	alSourcei(sourceId, AL_BUFFER, bufferId);
	alSourcei(sourceId, AL_LOOPING, AL_TRUE);
	al_CheckError();
	
	alSourcef(sourceId, AL_REFERENCE_DISTANCE, 1.0f); // world scale.
	alSourcef(sourceId, AL_ROLLOFF_FACTOR, 1.0f);
	
	alSourcePlay(sourceId);
	al_CheckError();
	
	[lblStatus setText:@"Play: OpenAL"];
}

static double _g_Frequencies[6] =
{
	277.183,
	293.665,
	311.127,
	329.628,
	349.228,
	369.994
};

static double g_Frequencies[6] =
{
	100.0,
	210.0,
	320.0,
	430.0,
	540.0,
	650.0
};

-(IBAction)touchNote1:(id)sender
{
	Synth_PlayNote(g_Frequencies[0]);
}

-(IBAction)touchNote2:(id)sender
{
	Synth_PlayNote(g_Frequencies[1]);
}

-(IBAction)touchNote3:(id)sender
{
	Synth_PlayNote(g_Frequencies[2]);
}

-(IBAction)touchNote4:(id)sender
{
	Synth_PlayNote(g_Frequencies[3]);
}

-(IBAction)touchNote5:(id)sender
{
	Synth_PlayNote(g_Frequencies[4]);
}

-(IBAction)touchNote6:(id)sender
{
	Synth_PlayNote(g_Frequencies[5]);
}

- (void)didReceiveMemoryWarning {
    [super didReceiveMemoryWarning];
}

- (void)dealloc {
	[m_AudioPlayer release];
    [super dealloc];
}

@end
