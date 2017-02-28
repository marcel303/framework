#include "config.h"

#if ENABLE_VIDEO

#include "framework.h"
#include "mediaplayer_new/MPVideoBuffer.h"
#include "video.h"
#include <atomic>

static SDL_mutex * s_avcodecMutex = nullptr;
static std::atomic_int s_numVideoThreads;
static const int kMaxVideoThreads = 64;

static int ExecMediaPlayerThread(void * param)
{
	MediaPlayer::Context * context = (MediaPlayer::Context*)param;

	SDL_LockMutex(context->mpTickMutex);

	context->mpThreadId = SDL_GetThreadID(nullptr);

	SDL_LockMutex(s_avcodecMutex);
	{
		context->hasBegun = context->mpContext.Begin(context->openParams.filename, false, true, context->openParams.yuv);
	}
	SDL_UnlockMutex(s_avcodecMutex);

	while (!context->stopMpThread)
	{
		const int delayMS = 10;

		if (context->hasBegun)
		{
			context->tick();
		}

		SDL_CondWaitTimeout(context->mpTickEvent, context->mpTickMutex, delayMS);
	}

	// media player thread is completely detached from the main thread at this point

	if (context)
	{
		const int t1 = SDL_GetTicks();

		SDL_LockMutex(s_avcodecMutex);
		{
			context->mpContext.End();
		}
		SDL_UnlockMutex(s_avcodecMutex);

		delete context;
		context = nullptr;

		const int t2 = SDL_GetTicks();

		logDebug("MP context end took %dms", t2 - t1);
	}

	s_numVideoThreads--;

	return 0;
}

void MediaPlayer::Context::tick()
{
	hasPresentedLastFrame = presentedLastFrame();

	if (hasPresentedLastFrame)
		return;

	mpContext.FillBuffers();

	mpContext.FillVideoBuffer();
}

bool MediaPlayer::Context::presentedLastFrame() const
{
	if (SDL_GetThreadID(nullptr) == mpThreadId)
	{
		// todo : actually check if the frame was presented

		return mpContext.HasBegun() && mpContext.Depleted();
	}
	else
	{
		return hasPresentedLastFrame;
	}
}

//

void MediaPlayer::openAsync(const char * filename, const bool yuv)
{
	Assert(context == nullptr);

	const int t1 = SDL_GetTicks();

	context = new Context();

	context->openParams.filename = filename;
	context->openParams.yuv = yuv;

	const int t2 = SDL_GetTicks();

	startMediaPlayerThread();

	const int t3 = SDL_GetTicks();

	logDebug("MP begin took %dms", t2 - t1);
	logDebug("MP thread start took %dms", t3 - t2);
}

void MediaPlayer::close()
{
	if (mpThread)
	{
		stopMediaPlayerThread();
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

void MediaPlayer::tick(Context * context)
{
	updateTexture();
}

bool MediaPlayer::isActive(Context * context) const
{
	return context != nullptr;
}

bool MediaPlayer::presentedLastFrame(Context * context) const
{
	return context && context->presentedLastFrame();
}

void MediaPlayer::seek(const double time)
{
	SDL_LockMutex(context->mpTickMutex);
	{
		context->mpContext.SeekToTime(time);
	}
	SDL_UnlockMutex(context->mpTickMutex);
}

void MediaPlayer::updateTexture()
{
	if (!context->hasBegun)
	{
		Assert(texture == 0);
		return;
	}

	Assert(context->mpContext.HasBegun());

	const double time = presentTime >= 0.0 ? presentTime : 0.0;

	MP::VideoFrame * videoFrame = nullptr;
	bool gotVideo = false;
	context->mpContext.RequestVideo(time, &videoFrame, gotVideo);

	if (gotVideo)
	{
		textureSx = videoFrame->m_width;
		textureSy = videoFrame->m_height;

		//logDebug("gotVideo. t=%06dms, sx=%d, sy=%d", int(time * 1000.0), textureSx, textureSy);

#if 1
		if (texture)
		{
			glDeleteTextures(1, &texture);
			texture = 0;
		}
#endif

		if (texture)
		{
			const void * source = videoFrame->m_frameBuffer;
			const int sx = videoFrame->m_width;
			const int sy = videoFrame->m_height;
			const GLenum format = GL_RGB;

			GLuint restoreTexture;
			glGetIntegerv(GL_TEXTURE_BINDING_2D, reinterpret_cast<GLint*>(&restoreTexture));
			GLint restoreUnpackAlignment;
			glGetIntegerv(GL_UNPACK_ALIGNMENT, &restoreUnpackAlignment);
			GLint restoreUnpackRowLength;
			glGetIntegerv(GL_UNPACK_ROW_LENGTH, &restoreUnpackRowLength);

			// copy image data

			glBindTexture(GL_TEXTURE_2D, texture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, sx);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				format,
				sx,
				sy,
				0,
				format,
				GL_UNSIGNED_BYTE,
				source);

			// restore previous OpenGL states

			glBindTexture(GL_TEXTURE_2D, restoreTexture);
			glPixelStorei(GL_UNPACK_ALIGNMENT, restoreUnpackAlignment);
			glPixelStorei(GL_UNPACK_ROW_LENGTH, restoreUnpackRowLength);
		}
		else
		{
			texture = createTextureFromRGB8(videoFrame->m_frameBuffer, videoFrame->m_width, videoFrame->m_height, true, true);
		}
	}
}

uint32_t MediaPlayer::getTexture() const
{
	return texture;
}

void MediaPlayer::startMediaPlayerThread()
{
	Assert(mpThread == nullptr);
	Assert(context->mpTickEvent == nullptr);
	Assert(context->mpTickMutex == nullptr);

	if (s_avcodecMutex == nullptr)
	{
		s_avcodecMutex = SDL_CreateMutex();

		std::atomic_init(&s_numVideoThreads, 0);
	}

	if (context->mpTickEvent == nullptr)
		context->mpTickEvent = SDL_CreateCond();
	if (context->mpTickMutex == nullptr)
		context->mpTickMutex = SDL_CreateMutex();

	if (mpThread == nullptr)
	{
		while (s_numVideoThreads == kMaxVideoThreads)
		{
			SDL_Delay(0);
		}

		s_numVideoThreads++;

		mpThread = SDL_CreateThread(ExecMediaPlayerThread, "MediaPlayerThread", context);
		SDL_DetachThread(mpThread);
	}
}

void MediaPlayer::stopMediaPlayerThread()
{
	Assert(mpThread != nullptr);

	const int t1 = SDL_GetTicks();

	if (mpThread != nullptr)
	{
		Assert(context->stopMpThread == false);
		
		context->stopMpThread = true;
		SDL_CondSignal(context->mpTickEvent);

		context = nullptr;
		mpThread = nullptr;

		// fixme : since we don't wait for the close operation to complete, we may run into trouble opening the same movie again
		//         if very little time passes between close and open. todo : ensure close has completed before allowing open operation
		//         or : fix avcodec so it can share opened files
	}

	const int t2 = SDL_GetTicks();

	logDebug("MP thread shutdown took %dms", t2 - t1);
}

#endif
