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
	#define ENABLE_KINECT1 0
#else
	#define ENABLE_KINECT1 0
#endif

#if ENABLE_KINECT1

#include "libfreenect.h"
#include "Vec3.h"

struct SDL_mutex;
struct SDL_Thread;

struct Kinect1
{
	const static int width = 640;
	const static int height = 480;
	
	freenect_context * context;
	freenect_device * device;
	
	bool bIsVideoInfrared = false;
	bool bUseRegistration = true;
	
	void * videoData[2];
	void * depthData[2];
	
	bool hasVideo;
	bool hasDepth;
	
	void * video;
	void * depth;
	
	freenect_led_options currentLed;
	bool ledIsDirty;
	
	float oldTiltAngle;
	float newTiltAngle;
	bool tiltAngleIsDirty;
	
	Vec3 mksAccel;
	Vec3 rawAccel;
	
	SDL_mutex * mutex;
	SDL_Thread * thread;
	bool stopThread;
	
	Kinect1()
		: context(nullptr)
		, device(nullptr)
		, videoData()
		, depthData()
		, hasVideo(false)
		, hasDepth(false)
		, video(nullptr)
		, depth(nullptr)
		, currentLed(LED_GREEN)
		, ledIsDirty(true)
		, oldTiltAngle(0.f)
		, newTiltAngle(0.f)
		, tiltAngleIsDirty(true)
		, mutex(nullptr)
		, thread(nullptr)
		, stopThread(false)
	{
	}
	
	~Kinect1();
	
	bool init();
	bool shut();
	
	void lockBuffers();
	void unlockBuffers();
	
	void threadInit();
	void threadShut();
	bool threadProcess();
	
	static int threadMain(void * userData);
	
	static void grabDepthFrame(freenect_device * dev, void * depth, uint32_t timestamp);
	static void grabVideoFrame(freenect_device * dev, void * video, uint32_t timestamp);
};

#else

struct Kinect1
{
	const static int width = 640;
	const static int height = 480;
	
	bool bIsVideoInfrared = false;
	bool bUseRegistration = true;
	
	bool hasVideo;
	bool hasDepth;
	
	void * video;
	void * depth;
	
	Kinect1()
		: hasVideo(false)
		, hasDepth(false)
		, video(nullptr)
		, depth(nullptr)
	{
	}
	
	bool init()
	{
		return true;
	}
	
	bool shut()
	{
		return true;
	}
	
	void lockBuffers()
	{
	}
	
	void unlockBuffers()
	{
	}
};

#endif
