#pragma once

#ifdef __MACOS__

#include <stdint.h>

struct SDL_cond;
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

struct MacWebcamContext
{
	MacWebcamImage * newImage;
	MacWebcamImage * oldImage;
	
	SDL_cond * cond;
	SDL_mutex * mutex;
	bool stop;
	
	uint64_t conversionTimeUsAvg;
	
	MacWebcamContext();
	~MacWebcamContext();
};

struct MacWebcam
{
	MacWebcamContext * context;
	
	MacWebcamImage * image;
	
	bool threaded;
	
	void * nonThreadedWebcamImpl;
	
	MacWebcam(const bool threaded = true);
	~MacWebcam();

	bool init();
	void shut();
	
	void tick();
};

#endif
