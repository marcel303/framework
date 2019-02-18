/*
	Copyright (C) 2017 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "framework.h"
#include "video.h"
#include <atomic>

#include "mediaplayer/MPVideoBuffer.h"
#include "StringEx.h"

static SDL_mutex * s_avcodecMutex = nullptr;
static std::atomic_int s_numVideoThreads;
static const int kMaxVideoThreads = 64;

static thread_local SDL_threadID s_mpThreadId = -1;

static int ExecMediaPlayerThread(void * param)
{
	MediaPlayer::Context * context = (MediaPlayer::Context*)param;
	
	{
		char threadName[1024];
		sprintf_s(threadName, sizeof(threadName), "Media Player (%s)", context->openParams.filename.c_str());
		cpuTimingSetThreadName(threadName);
	}

	SDL_LockMutex(context->mpTickMutex);

	s_mpThreadId = SDL_GetThreadID(nullptr);

	SDL_LockMutex(s_avcodecMutex);
	{
		context->hasBegun = context->mpContext.Begin(
			context->openParams.filename,
			context->openParams.enableAudioStream,
			context->openParams.enableVideoStream,
			context->openParams.outputMode,
			context->openParams.desiredAudioStreamIndex,
			context->openParams.audioOutputMode);
	}
	SDL_UnlockMutex(s_avcodecMutex);

	while (!context->stopMpThread)
	{
		SDL_UnlockMutex(context->mpTickMutex);
		
		if (context->hasBegun)
		{
			cpuTimingBlock(tickMediaPlayer);
			
			SDL_LockMutex(context->mpSeekMutex);
			
			context->tick();
			
			SDL_UnlockMutex(context->mpSeekMutex);
		}
		
		SDL_LockMutex(context->mpTickMutex);
		
		if (!context->stopMpThread)
			SDL_CondWait(context->mpTickEvent, context->mpTickMutex);
	}

	// media player thread is completely detached from the main thread at this point
	
	s_mpThreadId = -1;

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

		logDebug("MP context end took %dms", t2 - t1); (void)t1; (void)t2;
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

	mpContext.FillAudioBuffer();

	mpContext.FillVideoBuffer();
}

bool MediaPlayer::Context::presentedLastFrame() const
{
	if (SDL_GetThreadID(nullptr) == s_mpThreadId)
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

void MediaPlayer::openAsync(const OpenParams & openParams)
{
	Assert(context == nullptr);

	const int t1 = SDL_GetTicks();

	context = new Context();

	context->openParams = openParams;

	const int t2 = SDL_GetTicks();

	startMediaPlayerThread();

	const int t3 = SDL_GetTicks();

	logDebug("MP begin took %dms", t2 - t1); (void)t1; (void)t2;
	logDebug("MP thread start took %dms", t3 - t2); (void)t2; (void)t3;
}

void MediaPlayer::openAsync(const char * filename, const MP::OutputMode outputMode)
{
	OpenParams openParams;
	openParams.filename = filename;
	openParams.outputMode = outputMode;
	openParams.enableAudioStream = false;
	
	openAsync(openParams);
}

void MediaPlayer::close(const bool _freeTexture)
{
	if (mpThread)
	{
		stopMediaPlayerThread();
	}
	
	videoFrame = nullptr;
	
	audioChannelCount = -1;
	audioSampleRate = -1;
	
	if (_freeTexture)
	{
		const int t1 = SDL_GetTicks();

		freeTexture();

		const int t2 = SDL_GetTicks();

		logDebug("MP texture delete took %dms", t2 - t1); (void)t1; (void)t2;
	}
}

bool MediaPlayer::tick(Context * context, const bool wantsTexture)
{
	const bool gotVideoFrame = updateVideoFrame();
	
	if (wantsTexture)
	{
		if (gotVideoFrame)
		{
			updateTexture();
		}
	}
	else
	{
		freeTexture();
	}

	updateAudio();
	
	return gotVideoFrame;
}

bool MediaPlayer::isActive(Context * context) const
{
	return context != nullptr;
}

bool MediaPlayer::presentedLastFrame(Context * context) const
{
	return context && context->presentedLastFrame();
}

void MediaPlayer::seekToStart()
{
	SDL_LockMutex(context->mpTickMutex);
	{
		SDL_LockMutex(context->mpSeekMutex);
		{
			if (context->hasBegun)
			{
				context->mpContext.SeekToStart();
				
				presentTime = 0.0;
				audioTime = 0.0;
				
				SDL_CondSignal(context->mpTickEvent);
			}
		}
		SDL_UnlockMutex(context->mpSeekMutex);
	}
	SDL_UnlockMutex(context->mpTickMutex);
}

void MediaPlayer::seek(const double time, const bool nearest)
{
	SDL_LockMutex(context->mpTickMutex);
	{
		SDL_LockMutex(context->mpSeekMutex);
		{
			if (context->hasBegun)
			{
				double actualTime;
				
				context->mpContext.SeekToTime(time, nearest, actualTime);
				
				presentTime = actualTime;
				audioTime = actualTime;
				
				SDL_CondSignal(context->mpTickEvent);
			}
		}
		SDL_UnlockMutex(context->mpSeekMutex);
	}
	SDL_UnlockMutex(context->mpTickMutex);
}

bool MediaPlayer::updateVideoFrame()
{
	if (!context->hasBegun)
	{
		return false;
	}

	Assert(context->mpContext.HasBegun());

	const double time = presentTime >= 0.0 ? presentTime : 0.0;
	
	bool gotVideo = false;
	context->mpContext.RequestVideo(time, &videoFrame, gotVideo);
	
	if (gotVideo)
	{
		SDL_CondSignal(context->mpTickEvent);
		
		//logDebug("gotVideo. t=%06dms, sx=%d, sy=%d", int(time * 1000.0), textureSx, textureSy);
	}
	
	return gotVideo;
}

void MediaPlayer::updateTexture()
{
	if (!context->hasBegun)
	{
		return;
	}
	
	if (true)
	{
		Assert(videoFrame->m_isValidForRead);
		
		const void * bytes = nullptr;
		int sx = 0;
		int sy = 0;
		int pitch = 0;
		
		GX_TEXTURE_FORMAT format = GX_UNKNOWN_FORMAT;
		
		if (context->openParams.outputMode == MP::kOutputMode_PlanarYUV)
		{
			bytes = videoFrame->getY(sx, sy, pitch);
			
			format = GX_R8_UNORM;
		}
		else
		{
			bytes = videoFrame->m_frameBuffer;
			sx = videoFrame->m_width;
			sy = videoFrame->m_height;
			
			const int alignment = 16;
			const int alignmentMask = ~(alignment - 1);
			
			pitch = (((sx * 4) + alignment - 1) & alignmentMask) / 4;
			
			format = GX_RGBA8_UNORM;
		}
		
		if (texture == nullptr || sx != texture->sx || sy != texture->sy || format != texture->format)
		{
			freeTexture();
			
			texture = new GxTexture();
			texture->allocate(sx, sy, format, true, true);
			
			if (format == GX_R8_UNORM)
				texture->setSwizzle(0, 0, 0, GX_SWIZZLE_ONE);
		}
		
		if (texture != nullptr)
		{
			texture->upload(bytes, 1, pitch);
		}
	}
}

void MediaPlayer::freeTexture()
{
	if (texture != nullptr)
	{
		delete texture;
		texture = nullptr;
	}
}

uint32_t MediaPlayer::getTexture() const
{
	return texture ? texture->id : 0;
}

bool MediaPlayer::getVideoProperties(int & sx, int & sy, double & duration, double & sampleAspectRatio) const
{
	if (context->hasBegun)
	{
		sx = context->mpContext.GetVideoWidth();
		sy = context->mpContext.GetVideoHeight();
		duration = context->mpContext.GetDuration();
		sampleAspectRatio = context->mpContext.GetVideoSampleAspectRatio();
		
		return true;
	}
	else
	{
		sx = 0;
		sy = 0;
		duration = 0.0;
		
		return false;
	}
}

void MediaPlayer::updateAudio()
{
	if (!context->hasBegun)
	{
		return;
	}

	Assert(context->mpContext.HasBegun());

	if (audioChannelCount == -1)
	{
		audioChannelCount = context->mpContext.GetAudioChannelCount();
		audioSampleRate = context->mpContext.GetAudioFrameRate();
	}
}

bool MediaPlayer::getAudioProperties(int & channelCount, int & sampleRate) const
{
	if (audioChannelCount < 0 ||  audioSampleRate < 0)
	{
		return false;
	}
	else
	{
		channelCount = audioChannelCount;
		sampleRate = audioSampleRate;

		return true;
	}
}

int MediaPlayer::Provide(int numSamples, AudioSample* __restrict buffer)
{
	if (!context->hasBegun)
	{
		return 0;
	}
	
	bool gotAudio = false;

	double audioTime = this->audioTime;
	
	const int numChannels = context->mpContext.GetAudioChannelCount();
	
	int16_t * tempBuffer = (int16_t*)alloca(numSamples * numChannels * sizeof(int16_t));
	
	if (context->mpContext.RequestAudio(tempBuffer, numSamples, gotAudio, audioTime))
	{
		this->audioTime = audioTime;

		SDL_CondSignal(context->mpTickEvent);
	}
	
	if (numChannels == 0)
	{
		memset(buffer, 0, numSamples * sizeof(AudioSample));
	}
	else if (numChannels == 1)
	{
		for (int i = 0; i < numSamples; ++i)
		{
			buffer[i].channel[0] = tempBuffer[i];
			buffer[i].channel[1] = tempBuffer[i];
		}
	}
	else if (numChannels == 2)
	{
		for (int i = 0; i < numSamples; ++i)
		{
			buffer[i].channel[0] = tempBuffer[i * 2 + 0];
			buffer[i].channel[1] = tempBuffer[i * 2 + 1];
		}
	}
	else
	{
		// todo : handle multi-channel the correct way and apply the multi-channel to stereo mixing matrix (with a little help from avcodec)
		
		for (int i = 0; i < numSamples; ++i)
		{
			int sum = 0;
			
			for (int c = 0; c < numChannels; ++c)
				sum += tempBuffer[i * numChannels + c];
			
			buffer[i].channel[0] = sum;
			buffer[i].channel[1] = sum;
		}
	}

	return numSamples;
}

void MediaPlayer::startMediaPlayerThread()
{
	Assert(mpThread == nullptr);
	Assert(context->mpTickEvent == nullptr);
	Assert(context->mpTickMutex == nullptr);
	Assert(context->mpSeekMutex == nullptr);

	if (s_avcodecMutex == nullptr)
	{
		s_avcodecMutex = SDL_CreateMutex();

		std::atomic_init(&s_numVideoThreads, 0);
	}

	if (context->mpTickEvent == nullptr)
		context->mpTickEvent = SDL_CreateCond();
	if (context->mpTickMutex == nullptr)
		context->mpTickMutex = SDL_CreateMutex();
	if (context->mpSeekMutex == nullptr)
		context->mpSeekMutex = SDL_CreateMutex();

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
		
		SDL_LockMutex(context->mpTickMutex);
		{
			context->stopMpThread = true;
			
			SDL_CondSignal(context->mpTickEvent);
		}
		SDL_UnlockMutex(context->mpTickMutex);

		context = nullptr;
		mpThread = nullptr;
	}

	const int t2 = SDL_GetTicks();

	logDebug("MP thread shutdown took %dms", t2 - t1); (void)t1; (void)t2;
}
