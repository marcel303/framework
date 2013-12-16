#include <AudioToolbox/AudioToolbox.h>
#include "AudioSession.h"
#include "Log.h"

static bool g_PlayBackgroundMusic = false;
static OpenALState* g_OpenALState = 0;

static void AudioSession_InterruptionCallback(void* userData, UInt32 interruptionState);
static void Activate();
static void Deactivate();

//

void AudioSession_Initialize(bool playBackgroundMusic)
{
	OSStatus rtStatus;
	
	void* userData = 0;
	
	rtStatus = AudioSessionInitialize(
		NULL,
		NULL,
		AudioSession_InterruptionCallback,
		userData);

	if (rtStatus)
		LOG(LogLevel_Error, "Failed to initialize AudioSession: %d", (int)rtStatus);
	else
		LOG(LogLevel_Info, "AudioSession initialized");
	
	//
	
	g_PlayBackgroundMusic = playBackgroundMusic;
	
	UInt32 sessionCategory;
	
	if (playBackgroundMusic)
		sessionCategory = kAudioSessionCategory_AmbientSound; // Allows iPod music playback to be mixed.
	else
		sessionCategory = kAudioSessionCategory_SoloAmbientSound; // Does not allows iPod music playback to be mixed.

	rtStatus = AudioSessionSetProperty(
		kAudioSessionProperty_AudioCategory,
		sizeof (sessionCategory),
		&sessionCategory);
	
	if (rtStatus)
		LOG(LogLevel_Error, "Failed to set AudioSession catagory");
	else
		LOG(LogLevel_Info, "AudioSession catagory set");
	
	Activate();
}

void AudioSession_Shutdown()
{
	Deactivate();
}

void AudioSession_ManageOpenAL(OpenALState* openALState)
{
	g_OpenALState = openALState;
}

// AudioSession pause & resume functions.
// Required to handle interruptions due to calls, alarm clocks, etc.

void AudioSession_Pause()
{
	if (g_OpenALState)
	{
		// Suspend OpenAL context.
		alcSuspendContext(g_OpenALState->m_Context);
		
		// Disable OpenAL.
		alcMakeContextCurrent(NULL);
	}
	
	// Disable audio session.
	Deactivate();
}

void AudioSession_Resume()
{
	// Enable audio session.
	Activate();
	
	if (g_OpenALState)
	{
		// Enable OpenAL.
		alcMakeContextCurrent(g_OpenALState->m_Context);
		
		// Resume OpenAL context.
		alcProcessContext(g_OpenALState->m_Context);
	}
}

bool AudioSession_PlayBackgroundMusic()
{
	return g_PlayBackgroundMusic;
}

bool AudioSession_DetectbackgroundMusic()
{
	// detect if iTunes or another application is playing music in the background.
	
	AudioSessionInitialize(NULL, NULL, NULL, NULL);
	
	UInt32 isPlaying = 0;
	UInt32 size = sizeof(isPlaying); 
	
	AudioSessionGetProperty(kAudioSessionProperty_OtherAudioIsPlaying, &size, &isPlaying);
	
	return isPlaying;
}

static void AudioSession_InterruptionCallback(void* userData, UInt32 interruptionState)
{
	if (interruptionState == kAudioSessionBeginInterruption)
	{
		AudioSession_Pause();
	}
	else if (interruptionState == kAudioSessionEndInterruption)
	{
		AudioSession_Resume();
	}
}

static void Activate()
{
	OSStatus rtStatus = AudioSessionSetActive(YES);
	
	if (rtStatus)
		LOG(LogLevel_Error, "Failed to make AudioSession active");
	else
		LOG(LogLevel_Info, "AudioSession made active");
}

static void Deactivate()
{
	OSStatus rtStatus = AudioSessionSetActive(NO);
	
	if (rtStatus)
		LOG(LogLevel_Error, "Failed to make AudioSession inactive");
	else
		LOG(LogLevel_Info, "AudioSession made inactive");
}
