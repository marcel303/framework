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

#if ENABLE_KINECT2

#include "framework.h"
#include "kinect2.h"
#include "kinect2FrameListener.h"

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>

#include <string>

Kinect2::Kinect2()
	: freenect2(nullptr)
	, device(nullptr)
	, pipeline(nullptr)
	, listener(nullptr)
	, mutex(nullptr)
	, thread(nullptr)
	, stopThread(false)
	, isInit(false)
{
}

Kinect2::~Kinect2()
{
	shut();
}

bool Kinect2::init()
{
	libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Warning));
	
	//
	
	Assert(freenect2 == nullptr);
	freenect2 = new libfreenect2::Freenect2();
	
	//
	
	int types = 0;
	
	if (doVideo)
		types |= libfreenect2::Frame::Color;
	if (doDepth)
		types |= libfreenect2::Frame::Depth;
	
	Assert(listener == nullptr);
	listener = new DoubleBufferedFrameListener(types);
	
	//
	
	if (freenect2->enumerateDevices() == 0)
	{
		logDebug("no devices enumerated");
		return false;
	}
	
	std::string serial;
	
	if (serial.empty())
	{
		serial = freenect2->getDefaultDeviceSerialNumber();
	}
	
	if (serial.empty())
	{
		logDebug("no device found");
		return false;
	}
	
	Assert(pipeline == nullptr);
	pipeline = new libfreenect2::OpenCLPacketPipeline();
	
	Assert(device == nullptr);
	device = freenect2->openDevice(serial, pipeline);
	
	if (device == nullptr)
	{
		logError("failed to create device");
		return false;
	}
	
	//
	
	Assert(mutex == nullptr);
	mutex = SDL_CreateMutex();
	
	if (mutex == nullptr)
		return false;
	
	//
	
	Assert(thread == nullptr);
	thread = SDL_CreateThread(threadMain, "Kinect2 Thread", this);
	
	if (thread == nullptr)
		return false;
	
	//
	
	isInit = true;
	
	return true;
}

bool Kinect2::shut()
{
	if (thread != nullptr)
	{
		SDL_LockMutex(mutex);
		{
			stopThread = true;
		}
		SDL_UnlockMutex(mutex);
		
		SDL_WaitThread(thread, nullptr);
		thread = nullptr;
		
		stopThread = false;
	}
	
	if (mutex != nullptr)
	{
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
	}
	
	delete listener;
	listener = nullptr;
	
	delete device;
	device = nullptr;
	
	delete freenect2;
	freenect2 = nullptr;
	
	isInit = false;
	
	return true;
}

bool Kinect2::threadInit()
{
	const bool result = threadInitImpl();
	
	if (result == false)
	{
		threadShut();
	}
	
	return result;
}

bool Kinect2::threadInitImpl()
{
	if (doVideo)
		device->setColorFrameListener(listener);
	
	if (doDepth)
		device->setIrAndDepthFrameListener(listener);
	
	if (doVideo && doDepth)
	{
		if (!device->start())
		{
			logError("failed to start device");
			return false;
		}
	}
	else
	{
		if (!device->startStreams(doVideo, doDepth))
		{
			logError("failed to start device");
			return false;
		}
	}
	
	logDebug("device serial: %s", device->getSerialNumber().c_str());
	logDebug("device firmware: %s", device->getFirmwareVersion().c_str());
	
	return true;
}

void Kinect2::threadShut()
{
	if (device != nullptr)
	{
		device->stop();
		device->close();
	}
}

bool Kinect2::threadProcess()
{
	SDL_Delay(50);
	
	return true;
}

int Kinect2::threadMain(void * userData)
{
	Kinect2 * self = (Kinect2*)userData;
	
	if (self->threadInit() == false)
	{
		return -1;
	}
	
	for (;;)
	{
		bool stop = false;
		
		SDL_LockMutex(self->mutex);
		{
			stop = self->stopThread;
		}
		SDL_UnlockMutex(self->mutex);
		
		if (stop)
		{
			break;
		}
		
		if (self->threadProcess() == false)
		{
			logDebug("thread process failed");
		}
	}
	
	self->threadShut();
	
	return 0;
}

#endif
