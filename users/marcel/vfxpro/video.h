#pragma once

#include "mediaplayer_old/MPContext.h"
#include <stdint.h>

struct MediaPlayer
{
	class AudioOutput * audioOutput;
	struct MyAudioStream * audioStream;
	MP::Context mpContext;
	int sx;
	int sy;
	uint32_t texture;

	// threading related
	SDL_mutex * textureMutex;
	SDL_Thread * mpThread;
	volatile bool stopMpThread;
	uint8_t * videoData;
	int videoSx;
	int videoSy;
	bool videoIsDirty;

	MediaPlayer()
		: audioOutput(nullptr)
		, audioStream(nullptr)
		, sx(0)
		, sy(0)
		, texture(0)
		// threading related
		, textureMutex(0)
		, mpThread(0)
		, stopMpThread(false)
		, videoData(0)
		, videoSx(0)
		, videoSy(0)
		, videoIsDirty(false)
	{
	}

	~MediaPlayer()
	{
		if (isActive())
		{
			close();
		}
	}

	bool open(const char * filename);
	void close();
	void tick(const float dt);
	void draw();

	bool isActive() const;

	uint32_t getTexture();

	void startMediaPlayerThread();
	void stopMediaPlayerThread();
};
