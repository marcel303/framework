#pragma once

#include <stdint.h>

struct SDL_mutex;

struct MacWebcamImage
{
	int index;

	int sx;
	int sy;
	int pitch;
	
	uint8_t * data;
	
	MacWebcamImage(const int sx, const int sy);
	~MacWebcamImage();
};

struct MacWebcam
{
	void * webcamImpl;
	
	MacWebcamImage * image;
	MacWebcamImage * newImage;
	MacWebcamImage * oldImage;
	
	SDL_mutex * mutex;
	
	MacWebcam();
	~MacWebcam();

	bool init();
	void shut();
	
	void tick();
};
