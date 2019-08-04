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

#endif
