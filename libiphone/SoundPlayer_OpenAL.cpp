#include "SoundPlayer_OpenAL.h"
#include "System.h"

// todo: create SDL thread

SoundPlayer_OpenAL::SoundPlayer_OpenAL()
{
	mRes[0] = 0;
	mRes[1] = 0;
	mRes[2] = 0;
	mRes[3] = 0;
	mIsEnabled = true;
	mLoop = false;
}

void SoundPlayer_OpenAL::Initialize(bool playBackgroundMusic)
{
	// nop
}

void SoundPlayer_OpenAL::Shutdown()
{
	Stop();

	Close();
}

void SoundPlayer_OpenAL::Play(Res* res1, Res* res2, Res* res3, Res* res4, bool loop)
{
	if (mRes[0] == res1 && mRes[1] == res2 && mRes[2] == res3 && mRes[3] == res4 && mLoop == loop)
		return;

	Stop();

	Close();

	mRes[0] = res1;
	mRes[1] = res2;
	mRes[2] = res3;
	mRes[3] = res4;
	mLoop = loop;

	Open();
	
	if (mIsEnabled)
	{
		Start();
	}
}

void SoundPlayer_OpenAL::Start()
{
	LOG_DBG(__FUNCTION__, 0);
	
	mAudioOutput.Play();
}

void SoundPlayer_OpenAL::Stop()
{
	LOG_DBG(__FUNCTION__, 0);
	
	mAudioOutput.Stop();
}

bool SoundPlayer_OpenAL::HasFinished_get()
{
	return mAudioOutput.HasFinished_get();
}

void SoundPlayer_OpenAL::IsEnabled_set(bool enabled)
{
	if (enabled == mIsEnabled)
		return;

	mIsEnabled = enabled;

	if (!mIsEnabled)
	{
		Stop();
	}
	else
	{
		Start();
	}
}

void SoundPlayer_OpenAL::Volume_set(float volume)
{
	mAudioOutput.Volume_set(volume);
}

void SoundPlayer_OpenAL::Update()
{
	if (mRes[0] == 0 && mRes[1] == 0 && mRes[2] == 0 && mRes[3] == 0)
		return;
	
	if (mLoop && mAudioOutput.HasFinished_get())
	{
		LOG_DBG("OpenAL-Stream: loop", 0);

		Stop();

		Close();

		Open();

		Start();
	}
	
	if (mIsEnabled)
	{
	#ifdef BBOS
		mAudioOutput.Update(&mVorbisStreams[0]);
	#else
		mAudioOutput.Update(&mAudioMixer);
	#endif
	}
}

void SoundPlayer_OpenAL::Open()
{	
	mAudioOutput.Shutdown();

	int numChannels = 0;
	int sampleRate = 0;
	
	for (int i = 0; i < 4; ++i)
	{
		mAudioMixer.ClearStream(i);
		mVorbisStreams[i].Close();
		
		if (mRes[i] != 0)
		{
			mVorbisStreams[i].Open(g_System.GetResourcePath(mRes[i]->m_FileName).c_str(), mLoop);
			mAudioMixer.SetStream(i, &mVorbisStreams[i]);
								
			if (numChannels == 0)
			{
				numChannels = mVorbisStreams[i].mNumChannels;
				sampleRate = mVorbisStreams[i].mSampleRate;
			}
			else
			{
				Assert(numChannels == mVorbisStreams[i].mNumChannels);
				Assert(sampleRate == mVorbisStreams[i].mSampleRate);
			}
		}
	}
	
	if (numChannels == 0)
	{
		Close();
		return;
	}
	
	mAudioOutput.Initialize(numChannels, sampleRate);
}

void SoundPlayer_OpenAL::Close()
{
	mAudioOutput.Shutdown();
	
	for (int i = 0; i < 4; ++i)
	{
		mAudioMixer.ClearStream(i);
		mVorbisStreams[i].Close();
	}
}
