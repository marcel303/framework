#import <AudioToolbox/AudioToolbox.h>
#import <AVFoundation/AVFoundation.h>
#import "Benchmark.h"
#import "BinaryData.h"
#import "Debugging.h"
#import "ResIO.h"
#import "SoundPlayer.h"

SoundPlayer::SoundPlayer()
{
}

void SoundPlayer::Initialize(bool playBackgroundMusic)
{
	if (playBackgroundMusic == false)
	{
		UInt32 sessionCategory = kAudioSessionCategory_SoloAmbientSound; // Allows iPod music playback to be mixed.

		OSStatus status = AudioSessionSetProperty(
			kAudioSessionProperty_AudioCategory,
			sizeof (sessionCategory),
			&sessionCategory);
		
//		Assert(status == 0);
	}

	m_AudioPlayer = 0;
	m_Res = 0;
	m_Volume = 1.0f;
	m_Loop = false;
	m_IsEnabled = true;
	m_PlayBackgroundMusic = playBackgroundMusic;
}

void SoundPlayer::Shutdown()
{
	Stop();
}

void SoundPlayer::Play(Res* res, bool loop)
{
	UsingBegin(Benchmark bm("Starting sound playback"))
	{
		if (res == m_Res)
			return;
		
		Stop();
		
		m_Res = res;
		m_Loop = loop;
		
		if (m_IsEnabled)
		{
			Start();
		}
	}
	UsingEnd()
}

void SoundPlayer::Start()
{
	if (m_PlayBackgroundMusic)
		return;
	
	AVAudioPlayer* audioPlayer = nil;
	
	UsingBegin(Benchmark b("Creating AV audio player"))
	{
		// Create audio player.
	
		std::string bundleFileName = ResIO::GetBundleFileName(m_Res->m_FileName);
		
		NSURL* url = [NSURL fileURLWithPath:[NSString stringWithCString:bundleFileName.c_str() encoding:NSASCIIStringEncoding]];
		
		audioPlayer = [[AVAudioPlayer alloc] initWithContentsOfURL:url error:nil];
	}
	UsingEnd()
	
	UsingBegin(Benchmark b("Setting AV audio player options & play"))
	{
		// Set volume.
		
		[audioPlayer setVolume:m_Volume];
		
		// Set looping.
		
		if (m_Loop)
			[audioPlayer setNumberOfLoops:-1];
		else
			[audioPlayer setNumberOfLoops:0];
		
		// Play.
		
		[audioPlayer play];
		
		//
		
		m_AudioPlayer = audioPlayer;
	}
	UsingEnd()
}

void SoundPlayer::Stop()
{
	if (m_PlayBackgroundMusic)
		return;
	
	AVAudioPlayer* audioPlayer = (AVAudioPlayer*)m_AudioPlayer;
	
	[audioPlayer release];
	
	m_AudioPlayer = 0;
}

void SoundPlayer::IsEnabled_set(bool enabled)
{
	if (enabled == m_IsEnabled)
		return;
	
	m_IsEnabled = enabled;
	
	if (m_IsEnabled)
	{
		if (m_Res)
		{
			Start();
		}
	}
	else
	{
		Stop();
	}
}

void SoundPlayer::Volume_set(float volume)
{
	if (m_PlayBackgroundMusic)
		return;
	
	m_Volume = volume;
	
	AVAudioPlayer* audioPlayer = (AVAudioPlayer*)m_AudioPlayer;
	
	[audioPlayer setVolume:m_Volume];
}

void SoundPlayer::Update()
{
	// nop
}
