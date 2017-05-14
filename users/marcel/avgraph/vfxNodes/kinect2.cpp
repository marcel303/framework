#include "kinect2.h"

#if ENABLE_KINECT2

#include "framework.h"

#include <libfreenect2/libfreenect2.hpp>
#include <libfreenect2/frame_listener_impl.h>
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
	, hasVideo(false)
	, hasDepth(false)
	, video(nullptr)
	, depth(nullptr)
{
}

Kinect2::~Kinect2()
{
	shut();
}

bool Kinect2::init()
{
	//libfreenect2::setGlobalLogger(libfreenect2::createConsoleLogger(libfreenect2::Logger::Debug));
	
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
	
	//pipeline = new libfreenect2::CpuPacketPipeline();
	pipeline = new libfreenect2::OpenCLPacketPipeline();
	
	device = freenect2->openDevice(serial, pipeline);
	
	if (device == nullptr)
	{
		logError("failed to create device");
		shut();
		return false;
	}
	
	//
	
	threadInit();
	
	//
	
	bool stop = false;
	
	while (!stop)
	{
		threadProcess();
	}
	
	//
	
	threadShut();
	
	return true;
}

bool Kinect2::shut()
{
	return true;
}

void Kinect2::lockBuffers()
{
}

void Kinect2::unlockBuffers()
{
}

void Kinect2::threadInit()
{
	const bool doColor = false;
	const bool doDepth = true;
	
	int types = 0;
	
	if (doColor)
		types |= libfreenect2::Frame::Color;
	if (doDepth)
		types |= libfreenect2::Frame::Ir | libfreenect2::Frame::Depth;
	
	listener = new libfreenect2::SyncMultiFrameListener(types);
	
	device->setColorFrameListener(listener);
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
	
	delete pipeline;
	pipeline = nullptr;
	
	delete freenect2;
	freenect2 = nullptr;
}

bool Kinect2::threadProcess()
{
	libfreenect2::Frame undistorted(512, 424, 4);
	libfreenect2::Frame registered(512, 424, 4);

	libfreenect2::FrameMap frames;

	if (!listener->waitForNewFrame(frames, 10*1000)) // 10 seconds
	{
		logError("timeout!");
		shut();
		return false;
	}
	
	libfreenect2::Frame * rgb = frames[libfreenect2::Frame::Color];
	libfreenect2::Frame * ir = frames[libfreenect2::Frame::Ir];
	libfreenect2::Frame * depth = frames[libfreenect2::Frame::Depth];
	
	//registration->apply(rgb, depth, &undistorted, &registered);
	
	//if (depth->format == libfreenect2::Frame::Float)
	//if (rgb->format == libfreenect2::Frame::BGRX)
	{
		framework.process();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			//GLuint texture = createTextureFromRGBA8(rgb->data, rgb->width, rgb->height, true, true);
			GLuint texture = createTextureFromR32F(depth->data, depth->width, depth->height, true, true);
			//GLuint texture = createTextureFromR32F(registered.data, registered.width, registered.height, true, true);
			
			glBindTexture(GL_TEXTURE_2D, texture);
			GLint swizzleMask[4] = { GL_RED, GL_RED, GL_RED, GL_ONE };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
			
			gxSetTexture(texture);
			{
				const float s = std::pow(mouse.x / float(1024.f), 4.0);
				gxColor4f(s, s, s, 1.f);
				
				const float x1 = 0.f;
				const float y1 = 0.f;
				const float x2 = depth->width;
				const float y2 = depth->height;
				
				gxBegin(GL_QUADS);
				{
					gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
					gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
					gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
					gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
				}
				gxEnd();
			}
			gxSetTexture(0);
			
			glDeleteTextures(1, &texture);
		}
		framework.endDraw();
	}
	
	listener->release(frames);
}

#endif
