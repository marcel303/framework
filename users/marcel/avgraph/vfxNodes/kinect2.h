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

#define ENABLE_KINECT2 1

#if ENABLE_KINECT2

struct SDL_mutex;
struct SDL_Thread;

namespace libfreenect2
{
	class Frame;
	class FrameListener;
	class Freenect2;
	class Freenect2Device;
	class PacketPipeline;
	class Registration;
};

struct DoubleBufferedFrameListener;

struct Kinect2
{
	const static int width = 512; // fixme
	const static int height = 424;
	const static bool doVideo = true;
	const static bool doDepth = true;
	
	libfreenect2::Freenect2 * freenect2;
	libfreenect2::Freenect2Device * device;
	libfreenect2::PacketPipeline * pipeline;
	libfreenect2::Registration * registration;
	DoubleBufferedFrameListener * listener;
	
	SDL_mutex * mutex;
	SDL_Thread * thread;
	bool stopThread;
	
	bool isInit;
	
	Kinect2();
	~Kinect2();
	
	bool init();
	bool shut();
	
	bool threadInit();
	bool threadInitImpl();
	void threadShut();
	bool threadProcess();
	
	static int threadMain(void * userData);
};

#endif
