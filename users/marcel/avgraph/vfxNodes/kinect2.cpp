#include "kinect2.h"

#if ENABLE_KINECT2

#include "framework.h"

#include "kinect2FrameListener.h"

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/registration.h>
#include <libfreenect2/packet_pipeline.h>
#include <libfreenect2/logger.h>

#include <string>

Kinect2::Kinect2()
	: freenect2(nullptr)
	, device(nullptr)
	, pipeline(nullptr)
	, registration(nullptr)
	, listener(nullptr)
	, mutex(nullptr)
	, thread(nullptr)
	, stopThread(false)
{
}

Kinect2::~Kinect2()
{
	shut();
}

bool Kinect2::init()
{
	libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Warning));
	
	Assert(freenect2 == nullptr);
	freenect2 = new libfreenect2::Freenect2();
	
	if (freenect2->enumerateDevices() == 0)
	{
		logDebug("no devices enumerated");
		shut();
		return false;
	}
	
	std::string serial;
	
	if (serial.empty())
	{
		serial = freenect2->getDefaultDeviceSerialNumber();
	}
	
	Assert(pipeline == nullptr);
	pipeline = new libfreenect2::OpenCLPacketPipeline();
	
	Assert(device == nullptr);
	device = freenect2->openDevice(serial, pipeline);
	
	if (device == nullptr)
	{
		logError("failed to create device");
		shut();
		return false;
	}
	
	//
	
	Assert(mutex == nullptr);
	Assert(thread == nullptr);
	
	mutex = SDL_CreateMutex();
	thread = SDL_CreateThread(threadMain, "Kinect2 Thread", this);
	
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
		
		SDL_DestroyMutex(mutex);
		mutex = nullptr;
		
		stopThread = false;
	}
	
	return true;
}

void Kinect2::threadInit()
{
	const bool doColor = false;
	const bool doDepth = true;
	
	int types = 0;
	
	if (doColor)
		types |= libfreenect2::Frame::Color;
	if (doDepth)
		//types |= libfreenect2::Frame::Ir | libfreenect2::Frame::Depth;
		types |= libfreenect2::Frame::Depth;
	
	Assert(listener == nullptr);
	listener = new DoubleBufferedFrameListener(types);
	
	if (doColor)
		device->setColorFrameListener(listener);
	
	if (doDepth)
		device->setIrAndDepthFrameListener(listener);
	
	if (doColor && doDepth)
	{
		if (!device->start())
		{
			logError("failed to start device");
			shut();
			return;
		}
	}
	else
	{
		if (!device->startStreams(doColor, doDepth))
		{
			logError("failed to start device");
			shut();
			return;
		}
	}
	
	logDebug("device serial: %s", device->getSerialNumber().c_str());
	logDebug("device firmware: %s", device->getFirmwareVersion().c_str());
	
	Assert(registration == nullptr);
	registration = new libfreenect2::Registration(
		device->getIrCameraParams(),
		device->getColorCameraParams());
}

void Kinect2::threadShut()
{
	if (device != nullptr)
	{
		device->stop();
		device->close();
	}
	
	delete device;
	device = nullptr;
	
	delete listener;
	listener = nullptr;
	
	//delete pipeline;
	//pipeline = nullptr;
	
	delete freenect2;
	freenect2 = nullptr;
}

bool Kinect2::threadProcess()
{
	//libfreenect2::Frame undistorted(512, 424, 4);
	//libfreenect2::Frame registered(512, 424, 4);
	
	return true;
}

int Kinect2::threadMain(void * userData)
{
	Kinect2 * self = (Kinect2*)userData;
	
	self->threadInit();
	
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
		
		SDL_Delay(100);
	}
	
	self->threadShut();
	
	return 0;
}

#endif
