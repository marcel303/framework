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

	MediaPlayer()
		: audioOutput(nullptr)
		, audioStream(nullptr)
		, sx(0)
		, sy(0)
		, texture(0)
	{
	}

	~MediaPlayer()
	{
		close();
	}

	bool open(const char * filename);
	void close();
	void tick(const float dt);
	void draw();

	bool isActive() const;
};
