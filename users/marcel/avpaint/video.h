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

#pragma once

#include "audiostream/AudioStream.h"
#include "mediaplayer_new/MPContext.h"
#include <atomic>
#include <stdint.h>

struct MediaPlayer : public AudioStream
{
	struct OpenParams
	{
		OpenParams()
			: filename()
			, outputMode(MP::kOutputMode_RGBA)
			, enableAudioStream(true)
			, enableVideoStream(true)
		{
		}
		
		std::string filename;
		MP::OutputMode outputMode;
		bool enableAudioStream;
		bool enableVideoStream;
	};
	
	struct Context
	{
		Context()
			: mpTickEvent(nullptr)
			, mpTickMutex(nullptr)
			, mpThreadId(-1)
		#ifndef __WIN32__ // todo : do it like this on Win32 too
			, hasBegun(false)
			, stopMpThread(false)
			, hasPresentedLastFrame(false)
		#endif
		{
		#ifdef __WIN32__
			hasBegun = false;
			stopMpThread = false;
			hasPresentedLastFrame = false;
		#endif
		}

		~Context()
		{
			if (mpTickEvent)
			{
				SDL_DestroyCond(mpTickEvent);
				mpTickEvent = nullptr;
			}

			if (mpTickMutex)
			{
				SDL_DestroyMutex(mpTickMutex);
				mpTickMutex = nullptr;
			}
		}

		void tick();

		bool presentedLastFrame() const;

		OpenParams openParams;

		MP::Context mpContext;
		SDL_cond * mpTickEvent;
		SDL_mutex * mpTickMutex;
		SDL_threadID mpThreadId;

		// hacky messaging between threads
		std::atomic_bool hasBegun;
		std::atomic_bool stopMpThread;
		std::atomic_bool hasPresentedLastFrame;
	};

	Context * context;
	
	MP::VideoFrame * videoFrame;
	uint32_t texture;
	int textureSx;
	int textureSy;
	
	double presentTime;

	int audioChannelCount;
	int audioSampleRate;

	// threading related
	SDL_Thread * mpThread;

	MediaPlayer()
		: context(nullptr)
		, videoFrame(nullptr)
		, texture(0)
		, textureSx(0)
		, textureSy(0)
		, presentTime(-0.0001)
		, audioChannelCount(-1)
		, audioSampleRate(-1)
		// threading related
		, mpThread(0)
	{
	}

	~MediaPlayer()
	{
		close(true);
	}
	
	void openAsync(const OpenParams & openParams);
	void openAsync(const char * filename, const MP::OutputMode outputMode);
	void close(const bool freeTexture);
	bool tick(Context * context, const bool wantsTexture);

	bool isActive(Context * context) const;
	bool presentedLastFrame(Context * context) const;
	void seek(const double time);
	
	bool updateVideoFrame();
	void updateTexture();
	void freeTexture();
	uint32_t getTexture() const;
	bool getVideoProperties(int & sx, int & sy, double & duration) const;

	void updateAudio();
	bool getAudioProperties(int & channelCount, int & sampleRate) const;

	virtual int Provide(int numSamples, AudioSample* __restrict buffer) override;

	void startMediaPlayerThread();
	void stopMediaPlayerThread();
};
