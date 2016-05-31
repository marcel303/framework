#include "audiostream/AudioOutput.h"
#include "audiostream/AudioStream.h"
#include "framework.h"
#include "video.h"

static int ExecMediaPlayerThread(void * param)
{
	MediaPlayer * self = (MediaPlayer*)param;

	while (!self->stopMpThread)
	{
		const int delayMS = 10;

		SDL_Delay(delayMS);

		self->tick(delayMS / 1000.f);
	}

	return 0;
}

struct MyAudioStream : AudioStream
{
	AudioOutput * audioOutput;
	MP::Context * mpContext;

	struct TimingInfo
	{
		TimingInfo()
		{
			memset(this, 0, sizeof(*this));
		}

		double timeAudio;  ///< Time of decoder (audio PTS).
		double timeStream; ///< Time of stream sound.
	} m_timing[3];

	double m_timeCorrection;

	MyAudioStream()
		: AudioStream()
	{
		m_timeCorrection = 0.0;
	}

	virtual int Provide(int numSamples, AudioSample* __restrict buffer)
	{
		UpdateTiming(mpContext->GetAudioTime());

		bool gotAudio = false;

		mpContext->RequestAudio((int16_t*)buffer, numSamples, gotAudio);

		if (gotAudio)
			return numSamples;
		else
			return 0;
	}

	void UpdateTiming(double timeAudio)
	{
		m_timing[2] = m_timing[1];
		m_timing[1] = m_timing[0];

		m_timing[0].timeAudio = timeAudio;
		m_timing[0].timeStream = audioOutput->PlaybackPosition_get();

		m_timeCorrection = m_timing[2].timeAudio - m_timing[0].timeStream;

	#if 0
		logDebug("sync = %03.3f. phys = %03.3f. correction = %+03.3f",
			(float)m_timing[0].timeAudio,
			(float)m_timing[0].timeStream,
			(float)m_timeCorrection);
	#endif
	}

	double GetTime() const
	{
		return audioOutput->PlaybackPosition_get() + m_timeCorrection;
	}
};


bool MediaPlayer::open(const char * filename)
{
	bool result = true;

	if (result)
	{
		if (!mpContext.Begin(filename))
		{
			result = false;
		}
	}

	if (result)
	{
		AudioOutput_OpenAL * audioOutputOpenAL = new AudioOutput_OpenAL();
		audioOutput = audioOutputOpenAL;

		if (!audioOutputOpenAL->Initialize(2, mpContext.GetAudioFrameRate(), 1 << 13))
		{
			result = false;
		}
		else
		{
			audioOutputOpenAL->Volume_set(0.f);
		}
	}

	if (result)
	{
		audioStream = new MyAudioStream();
		audioStream->mpContext = &mpContext;
		audioStream->audioOutput = audioOutput;
	}

	if (result)
	{
		if (!mpContext.FillBuffers())
		{
			result = false;
		}
	}

	if (result)
	{
		audioOutput->Play();

		startMediaPlayerThread();
	}

	if (!result)
	{
		close();
	}

	return result;
}

void MediaPlayer::close()
{
	if (mpThread)
	{
		stopMediaPlayerThread();
	}

	if (texture)
	{
		glDeleteTextures(1, &texture);
		texture = 0;
	}

	if (audioStream)
	{
		delete audioStream;
		audioStream = nullptr;
	}

	if (audioOutput)
	{
		delete audioOutput;
		audioOutput = nullptr;
	}

	mpContext.End();
}

void MediaPlayer::tick(const float dt)
{
	if (!isActive())
		return;

	//logDebug("PlaybackPosition: %g", (float)audioOutput->PlaybackPosition_get());
	if (!mpContext.FillBuffers())
		return; // todo : check if all packets have been consumed!

	audioOutput->Update(audioStream);

	double time = audioStream->GetTime();
	MP::VideoFrame * videoFrame = nullptr;
	bool gotVideo = false;
	mpContext.RequestVideo(time, &videoFrame, gotVideo);
	if (gotVideo)
	{
		SDL_LockMutex(textureMutex);
		{
			delete [] videoData;
			videoData;

			videoData = new uint8_t[videoFrame->m_width * videoFrame->m_height * 3];
			memcpy(videoData, videoFrame->m_frameBuffer, videoFrame->m_width * videoFrame->m_height * 3);
			videoSx = videoFrame->m_width;
			videoSy = videoFrame->m_height;
			videoIsDirty = true;
			
			sx = videoFrame->m_width;
			sy = videoFrame->m_height;
		}
		SDL_UnlockMutex(textureMutex);
		//logDebug("gotVideo. t=%06dms", int(time * 1000.0));
	}
}

void MediaPlayer::draw()
{
	const uint32_t texture = getTexture();

	if (texture)
	{
		gxSetTexture(texture);
		drawRect(0, sy, sx, 0);
		gxSetTexture(0);
	}
}

bool MediaPlayer::isActive() const
{
	return mpContext.HasBegun();
}

uint32_t MediaPlayer::getTexture()
{
	if (videoIsDirty)
	{
		SDL_LockMutex(textureMutex);
		{
			videoIsDirty = false;

			if (texture)
				glDeleteTextures(1, &texture);
			texture = createTextureFromRGB8(videoData, videoSx, videoSy, true, true);
		}
		SDL_UnlockMutex(textureMutex);
	}

	return texture;
}

void MediaPlayer::startMediaPlayerThread()
{
	Assert(mpThread == nullptr);

	if (mpThread == nullptr)
	{
		textureMutex = SDL_CreateMutex();

		mpThread = SDL_CreateThread(ExecMediaPlayerThread, "MediaPlayerThread", this);
	}
}

void MediaPlayer::stopMediaPlayerThread()
{
	Assert(mpThread != nullptr);

	if (mpThread != nullptr)
	{
		stopMpThread = true;

		SDL_WaitThread(mpThread, nullptr);
		mpThread = nullptr;

		stopMpThread = false;
	}

	if (textureMutex != nullptr)
	{
		SDL_DestroyMutex(textureMutex);
		textureMutex = nullptr;
	}
}
