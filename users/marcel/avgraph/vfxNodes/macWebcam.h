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

#ifdef MACOS

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
