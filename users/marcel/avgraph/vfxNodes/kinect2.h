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
