#pragma once

#define ENABLE_KINECT2 1

#if ENABLE_KINECT2

namespace libfreenect2
{
	class FrameListener;
	class Freenect2;
	class Freenect2Device;
	class PacketPipeline;
	class Registration;
	class SyncMultiFrameListener;
};

struct Kinect2
{
	const static int width = 512; // fixme
	const static int height = 424;
	
	libfreenect2::Freenect2 * freenect2;
	libfreenect2::Freenect2Device * device;
	libfreenect2::PacketPipeline * pipeline;
	libfreenect2::Registration * registration;
	libfreenect2::SyncMultiFrameListener * listener;
	
	bool hasVideo;
	bool hasDepth;
	
	void * video;
	void * depth;
	
	Kinect2();
	~Kinect2();
	
	bool init();
	bool shut();
	
	void lockBuffers();
	void unlockBuffers();
	
	void threadInit();
	void threadShut();
	bool threadProcess();
};

#endif
