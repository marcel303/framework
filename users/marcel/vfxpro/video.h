#pragma once

#include "config.h"

#if ENABLE_VIDEO

#include "mediaplayer/MPContext.h"
#include <stdint.h>

struct MediaPlayer
{
	struct Context
	{
		Context()
			: mpTickEvent(nullptr)
			, mpTickMutex(nullptr)
			, mpThreadId(-1)
			, hasBegun(false)
			, stopMpThread(false)
			, hasPresentedLastFrame(false)
		{
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

		struct OpenParams
		{
			std::string filename;
			bool yuv;
		};

		OpenParams openParams;

		MP::Context mpContext;
		SDL_cond * mpTickEvent;
		SDL_mutex * mpTickMutex;
		SDL_threadID mpThreadId;

		// hacky messaging between threads
		volatile bool hasBegun;
		volatile bool stopMpThread;
		volatile bool hasPresentedLastFrame;
	};

	Context * context;

	uint32_t texture;
	int textureSx;
	int textureSy;
	double presentTime;

	// threading related
	SDL_Thread * mpThread;

	MediaPlayer()
		: context(nullptr)
		, texture(0)
		, textureSx(0)
		, textureSy(0)
		, presentTime(-1.0)
		// threading related
		, mpThread(0)
	{
	}

	~MediaPlayer()
	{
		close();
	}

	void openAsync(const char * filename, const bool yuv);
	void close();
	void tick(Context * context);

	bool isActive(Context * context) const;
	bool presentedLastFrame(Context * context) const;
	void seek(const double time);

	void updateTexture();
	uint32_t getTexture() const;
	bool getVideoProperties(int & sx, int & sy, double & duration) const;

	void startMediaPlayerThread();
	void stopMediaPlayerThread();
};

#endif
