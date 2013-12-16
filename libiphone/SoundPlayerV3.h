/* Copyright (C) 2008 Apple Inc. All Rights Reserved. */

#pragma once

#include <CoreAudio/CoreAudioTypes.h>
#include <AudioToolbox/AudioToolbox.h>
#include "Res.h"
#include "ISoundPlayer.h"

//==================================================================================================
//	Sound Engine
//==================================================================================================
/*!
    @enum SoundEngine error codes
    @abstract   These are the error codes returned from the SoundEngine API.
    @constant   kSoundEngineErrUnitialized 
		The SoundEngine has not been initialized. Use SoundEngine_Initialize().
    @constant   kSoundEngineErrInvalidID 
		The specified EffectID was not found. This can occur if the effect has not been loaded, or
		if an unloaded is trying to be accessed.
    @constant   kSoundEngineErrFileNotFound 
		The specified file was not found.
    @constant   kSoundEngineErrInvalidFileFormat 
		The format of the file is invalid. Effect data must be little-endian 8 or 16 bit LPCM.
    @constant   kSoundEngineErrDeviceNotFound 
		The output device was not found.

*/
enum
{
		kSoundEngineErrUnitialized			= 1,
//		kSoundEngineErrInvalidID			= 2,
		kSoundEngineErrFileNotFound			= 3,
		kSoundEngineErrInvalidFileFormat	= 4,
		kSoundEngineErrDeviceNotFound		= 5
};

/*!
    @function       SoundEngine_LoadBackgroundMusicTrack
    @abstract       Tells the background music player which file to play
    @param          inPath
                        The absolute path to the file to play.
    @param          inAddToQueue
                        If true, file will be added to the current background music queue. If
						false, queue will be cleared and only loop the specified file.
    @param          inLoadAtOnce
                        If true, file will be loaded completely into memory. If false, data will be 
						streamed from the file as needed. For games without large memory pressure and/or
						small background music files, this can save memory access and improve power efficiency
	@result         A OSStatus indicating success or failure.
*/
OSStatus  SoundEngine_LoadBackgroundMusicTrack(const char* inPath, Boolean inAddToQueue, Boolean inLoadAtOnce);

/*!
    @function       SoundEngine_UnloadBackgroundMusicTrack
    @abstract       Tells the background music player to release all resources and stop playing.
    @result         A OSStatus indicating success or failure.
*/
OSStatus  SoundEngine_UnloadBackgroundMusicTrack();

/*!
    @function       SoundEngine_StartBackgroundMusic
    @abstract       Tells the background music player to start playing.
    @result         A OSStatus indicating success or failure.
*/
OSStatus  SoundEngine_StartBackgroundMusic();

/*!
    @function       SoundEngine_StopBackgroundMusic
    @abstract       Tells the background music player to stop playing.
    @param          inAddToQueue
                        If true, playback will stop when all tracks have completed. If false, playback
						will stop immediately.
    @result         A OSStatus indicating success or failure.
*/
OSStatus  SoundEngine_StopBackgroundMusic(Boolean inStopAtEnd);

/*!
    @function       SoundEngine_SetBackgroundMusicVolume
    @abstract       Sets the volume for the background music player
    @param          inValue
                        A Float32 that represents the level. The range is between 0.0 and 1.0 (inclusive).
    @result         A OSStatus indicating success or failure.
*/
OSStatus  SoundEngine_SetBackgroundMusicVolume(Float32 inValue);
	
/*!
	 @function       SoundEngine_GetBackgroundMusicVolume
	 @abstract       Gets the volume for the background music player
	 @result         A Float32 representing the background music player volume, or 0 if it's not enabled
*/
Float32  SoundEngine_GetBackgroundMusicVolume();

Boolean SoundEngine_HasEnded();

//

class SoundPlayerV3 : public ISoundPlayer
{
public:
	SoundPlayerV3();
	
	void Initialize(bool playBackgroundMusic);
	void Shutdown();
	
	void Play(Res* res1, Res* res2, Res* res3, Res* res4, bool loop);
	void Start();	
	void Stop();	
	bool HasFinished_get();
	
	void IsEnabled_set(bool enabled);
	void Volume_set(float volume);
	void ChannelVolume_set(int channelIdx, float volume);
	
	void Update();
	
private:
	Res* m_Res;
	bool m_IsEnabled;
	float m_Volume;
	bool m_Loop;
	bool m_PlayBackgroundMusic;
};	
