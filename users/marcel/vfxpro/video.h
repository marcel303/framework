#pragma once

#include "mediaplayer_old/MPContext.h"
#include <stdint.h>

struct MediaPlayer
{
	struct Context
	{
		Context()
			: mpTickEvent(nullptr)
			, mpTickMutex(nullptr)
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

		MP::Context mpContext;
		SDL_cond * mpTickEvent;
		SDL_mutex * mpTickMutex;
	};

	Context * context;

	class AudioOutput * audioOutput;
	struct MyAudioStream * audioStream;
	int sx;
	int sy;
	uint32_t texture;

	// threading related
	SDL_mutex * textureMutex;
	SDL_Thread * mpThread;
	volatile bool stopMpThread;
	volatile bool stopMpThreadDone;
	uint8_t * videoData;
	int videoSx;
	int videoSy;
	bool videoIsDirty;

	MediaPlayer()
		: context(nullptr)
		, audioOutput(nullptr)
		, audioStream(nullptr)
		, sx(0)
		, sy(0)
		, texture(0)
		// threading related
		, textureMutex(0)
		, mpThread(0)
		, stopMpThread(false)
		, stopMpThreadDone(false)
		, videoData(0)
		, videoSx(0)
		, videoSy(0)
		, videoIsDirty(false)
	{
	}

	~MediaPlayer()
	{
		close();
	}

	bool open(const char * filename);
	void close();
	void tick(Context * context, const float dt);
	void draw();

	bool isActive(Context * context) const;

	uint32_t getTexture();

	void startMediaPlayerThread();
	void stopMediaPlayerThread();
};
