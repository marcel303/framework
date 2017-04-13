#pragma once

#include <string>

struct MediaPlayer;
class Surface;

struct VideoLoop
{
	std::string filename;
	MediaPlayer * mediaPlayer;
	MediaPlayer * mediaPlayer2;
	Surface * firstFrame;
	bool captureFirstFrame;
	
	VideoLoop(const char * filename, const bool captureFirstFrame);
	~VideoLoop();
	
	void tick(const float dt);
	
	uint32_t getTexture() const;
	uint32_t getFirstFrameTexture() const;
	bool getVideoProperties(int & sx, int & sy, double & duration) const;
};
