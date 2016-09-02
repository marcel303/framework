#include "framework.h"
#include "video.h"

static int ExecMediaPlayerThread(void * param)
{
	MediaPlayer * self = (MediaPlayer*)param;

	MediaPlayer::Context * context = self->context;

	SDL_LockMutex(context->mpTickMutex);

	while (!self->stopMpThread)
	{
		const int delayMS = 10;

		self->tick(context, delayMS / 1000.0);

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
		startMediaPlayerThread();
	}

	const int t3 = SDL_GetTicks();

	logDebug("MP begin took %dms", t2 - t1);
	logDebug("MP thread start took %dms", t3 - t2);

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
			context = nullptr;
		}
	}

	const int t1 = SDL_GetTicks();

	if (texture)
	{
		glDeleteTextures(1, &texture);
		texture = 0;
	}

	const int t2 = SDL_GetTicks();

	logDebug("MP texture delete took %dms", t2 - t1);
}

void MediaPlayer::tick(Context * context, const double dt)
{
	if (!isActive(context))
		return;

	if (seekTime >= 0.0)
	{
		SDL_LockMutex(context->mpBufferLock);
		{
			context->mpContext.SeekToTime(seekTime);
		}
		SDL_UnlockMutex(context->mpBufferLock);

		seekTime = -1.0;
	}

	if (presentedLastFrame(context))
		return;

	context->mpContext.FillBuffers();

	// todo : should be more fine grained. reduce the lock to insertion into the buffer only
	SDL_LockMutex(context->mpBufferLock);
	{
		context->mpContext.FillVideoBuffer();
	}
	SDL_UnlockMutex(context->mpBufferLock);
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

void MediaPlayer::seek(const double time)
{
	SDL_LockMutex(context->mpTickMutex);
	{
		seekTime = time;

		SDL_CondSignal(context->mpTickEvent);
	}
	SDL_UnlockMutex(context->mpTickMutex);
}

uint32_t MediaPlayer::updateTexture()
{
	Assert(context->mpContext.HasBegun());

	SDL_LockMutex(context->mpBufferLock);
	{
		const double time = presentTime >= 0.0 ? presentTime : 0.0;

		MP::VideoFrame * videoFrame = nullptr;
		bool gotVideo = false;
		context->mpContext.RequestVideo(time, &videoFrame, gotVideo);

		//if (gotVideo)
		if (videoFrame)
		{
			textureSx = videoFrame->m_width;
			textureSy = videoFrame->m_height;

			//logDebug("gotVideo. t=%06dms, sx=%d, sy=%d", int(time * 1000.0), textureSx, textureSy);

			if (texture)
			{
				glDeleteTextures(1, &texture);
				texture = 0;
			}

			texture = createTextureFromRGB8(videoFrame->m_frameBuffer, videoFrame->m_width, videoFrame->m_height, true, true);
		}
	}
	SDL_UnlockMutex(context->mpBufferLock);

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
}
