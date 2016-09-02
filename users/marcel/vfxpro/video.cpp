#include "audiostream/AudioOutput.h"
#include "audiostream/AudioStream.h"
#include "framework.h"
#include "video.h"

/*

todo : ideal processing

codec thread:
	fill buffers
	decode as many video frames as possible, store in video buffer
	decode as much audio as possible (deplete packet queue), consume immediately
	when video buffers are full, wait until a frame is consumed

main thread
	update media time
	consume or reuse video buffer. create new texture on change

*/

static int ExecMediaPlayerThread(void * param)
{
	MediaPlayer * self = (MediaPlayer*)param;

	MediaPlayer::Context * context = self->context;

	SDL_LockMutex(context->mpTickMutex);

	while (!self->stopMpThread)
	{
		const int delayMS = 10;

		self->tick(context, delayMS / 1000.f);

		SDL_CondWaitTimeout(context->mpTickEvent, context->mpTickMutex, delayMS);
	}

	SDL_MemoryBarrierRelease();
	self->stopMpThreadDone = true;

	if (context)
	{
		const int t1 = SDL_GetTicks();

		context->mpContext.End();

		delete context;
		context = nullptr;

		const int t2 = SDL_GetTicks();

		logDebug("MP context end took %dms", t2 - t1);
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
		if (mpContext->HasAudioStream())
			UpdateTiming(mpContext->GetAudioTime());

		bool gotAudio = false;

		if (mpContext->HasAudioStream())
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
	Assert(context == nullptr);

	bool result = true;

	const int t1 = SDL_GetTicks();

	if (result)
	{
		context = new Context();

		if (!context->mpContext.Begin(filename, false, true))
		{
			result = false;
		}
	}

	const int t2 = SDL_GetTicks();

	if (result)
	{
		AudioOutput_OpenAL * audioOutputOpenAL = new AudioOutput_OpenAL();
		audioOutput = audioOutputOpenAL;

		const int sampleRate = context->mpContext.HasAudioStream() ? context->mpContext.GetAudioFrameRate() : 11000;

		if (!audioOutputOpenAL->Initialize(2, sampleRate, 1 << 13))
		{
			result = false;
		}
		else
		{
			audioOutputOpenAL->Volume_set(0.f);
		}
	}

	const int t3 = SDL_GetTicks();

	if (result)
	{
		audioStream = new MyAudioStream();
		audioStream->mpContext = &context->mpContext;
		audioStream->audioOutput = audioOutput;
	}

	if (result)
	{
		if (!context->mpContext.FillBuffers())
		{
			result = false;
		}
	}

	const int t4 = SDL_GetTicks();

	if (result)
	{
		audioOutput->Play();

		startMediaPlayerThread();
	}

	const int t5 = SDL_GetTicks();

	logDebug("MP begin took %dms", t2 - t1);
	logDebug("audio output init took %dms", t3 - t2);
	logDebug("MP fill took %dms", t4 - t3);
	logDebug("MP thread start took %dms", t5 - t4);

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
	else
	{
		if (context)
		{
			delete context;
		}
	}

	context = nullptr;

	const int t1 = SDL_GetTicks();

	if (texture)
	{
		glDeleteTextures(1, &texture);
		texture = 0;
	}

	delete [] videoData;
	videoData = nullptr;
	
	videoIsDirty = false;

	if (audioStream)
	{
		delete audioStream;
		audioStream = nullptr;
	}

	const int t2 = SDL_GetTicks();

	if (audioOutput)
	{
		delete audioOutput;
		audioOutput = nullptr;
	}

	const int t3 = SDL_GetTicks();

	logDebug("MP texture delete took %dms", t2 - t1);
	logDebug("MP audio output delete took %dms", t3 - t2);
}

void MediaPlayer::tick(Context * context, const float dt)
{
	if (!isActive(context))
		return;

	if (seekTime >= 0.f)
	{
		context->mpContext.SeekToTime(seekTime);

		seekTime = -1.f;
	}

	if (presentedLastFrame(context))
		return;

	context->mpContext.FillBuffers();

	//logDebug("PlaybackPosition: %g", (float)audioOutput->PlaybackPosition_get());
	audioOutput->Update(audioStream);

	double time = presentTime >= 0.f ? presentTime : (audioStream->GetTime() * speed);

	MP::VideoFrame * videoFrame = nullptr;
	bool gotVideo = false;
	context->mpContext.RequestVideo(time, &videoFrame, gotVideo);
	if (gotVideo)
	{
		SDL_LockMutex(textureMutex);
		{
			delete [] videoData;
			videoData = nullptr;

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

bool MediaPlayer::isActive(Context * context) const
{
	return context && context->mpContext.HasBegun();
}

bool MediaPlayer::presentedLastFrame(Context * context) const
{
	// todo : actually check if the frame was presented

	return context && context->mpContext.HasBegun() && context->mpContext.Depleted();
}

void MediaPlayer::seek(const float time)
{
	SDL_LockMutex(context->mpTickMutex);
	{
		seekTime = time;

		SDL_CondSignal(context->mpTickEvent);
	}
	SDL_UnlockMutex(context->mpTickMutex);
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
	Assert(context->mpTickEvent == nullptr);
	Assert(context->mpTickMutex == nullptr);

	if (context->mpTickEvent == nullptr)
		context->mpTickEvent = SDL_CreateCond();
	if (context->mpTickMutex == nullptr)
		context->mpTickMutex = SDL_CreateMutex();

	if (mpThread == nullptr)
	{
		textureMutex = SDL_CreateMutex();

		mpThread = SDL_CreateThread(ExecMediaPlayerThread, "MediaPlayerThread", this);
		//SDL_DetachThread(mpThread);
	}
}

void MediaPlayer::stopMediaPlayerThread()
{
	Assert(mpThread != nullptr);

	const int t1 = SDL_GetTicks();

	if (mpThread != nullptr)
	{
		Assert(stopMpThread == false);
		Assert(stopMpThreadDone == false);
		
		stopMpThread = true;
		SDL_CondSignal(context->mpTickEvent);

		while (!stopMpThreadDone)
		{
			SDL_Delay(0);
		}

		SDL_WaitThread(mpThread, nullptr);
		mpThread = nullptr;

		stopMpThread = false;
		stopMpThreadDone = false;
	}

	const int t2 = SDL_GetTicks();

	logDebug("MP thread shutdown took %dms", t2 - t1);

	if (textureMutex != nullptr)
	{
		SDL_DestroyMutex(textureMutex);
		textureMutex = nullptr;
	}
}
