#include "framework.h"
#include "kinect2FrameListener.h"

using namespace libfreenect2;

DoubleBufferedFrameListener::DoubleBufferedFrameListener(int _frameTypes)
	: mutex(nullptr)
	, frameTypes(_frameTypes)
	, video(nullptr)
	, depth(nullptr)
{
	mutex = SDL_CreateMutex();
}

DoubleBufferedFrameListener::~DoubleBufferedFrameListener()
{
	delete video;
	video = nullptr;
	
	delete depth;
	depth = nullptr;
	
	SDL_DestroyMutex(mutex);
	mutex = nullptr;
}

bool DoubleBufferedFrameListener::onNewFrame(Frame::Type type, Frame * frame)
{
	if ((frameTypes & type) == 0)
	{
		return false;
	}

	lockBuffers();
	{
		if (type == Frame::Color)
		{
			delete video;
			video = nullptr;

			video = frame;
		}
		else if (type == Frame::Depth)
		{
			delete depth;
			depth = nullptr;

			depth = frame;
		}
		else
		{
			Assert(false);
			
			delete frame;
			frame = nullptr;
		}
	}
	unlockBuffers();

	return true;
}

void DoubleBufferedFrameListener::lockBuffers()
{
	SDL_LockMutex(mutex);
}

void DoubleBufferedFrameListener::unlockBuffers()
{
	SDL_UnlockMutex(mutex);
}
