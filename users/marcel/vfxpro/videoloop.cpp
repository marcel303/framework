#include "framework.h"
#include "video.h"
#include "videoloop.h"

VideoLoop::VideoLoop(const char * _filename, const bool _captureFirstFrame)
	: filename()
	, mediaPlayer(nullptr)
	, mediaPlayer2(nullptr)
	, firstFrame(nullptr)
	, captureFirstFrame(false)
{
	filename = _filename;
	captureFirstFrame = _captureFirstFrame;
}

VideoLoop::~VideoLoop()
{
	delete firstFrame;
	firstFrame = nullptr;
	
	delete mediaPlayer;
	mediaPlayer = nullptr;
	
	delete mediaPlayer2;
	mediaPlayer2 = nullptr;
}

void VideoLoop::tick(const float dt)
{
	if (mediaPlayer2 == nullptr)
	{
		if (mediaPlayer == nullptr || mediaPlayer->presentTime >= 1.0)
		{
			mediaPlayer2 = new MediaPlayer();
			mediaPlayer2->openAsync(filename.c_str(), false);
		}
	}
	
	if (mediaPlayer && mediaPlayer->context->hasPresentedLastFrame)
	{
		delete firstFrame;
		firstFrame = nullptr;
		
		delete mediaPlayer;
		mediaPlayer = nullptr;
	}
	
	if (mediaPlayer == nullptr)
	{
		mediaPlayer = mediaPlayer2;
		mediaPlayer2 = nullptr;
	}
	
	if (mediaPlayer)
	{
		mediaPlayer->tick(mediaPlayer->context);
		
		if (firstFrame == nullptr)
		{
			int sx;
			int sy;
			double duration;
			
			if (mediaPlayer->getVideoProperties(sx, sy, duration) && mediaPlayer->getTexture())
			{
				firstFrame = new Surface(sx, sy, false);
				
				pushSurface(firstFrame);
				{
					gxSetTexture(mediaPlayer->getTexture());
					setColor(colorWhite);
					gxBegin(GL_QUADS);
					{
						gxTexCoord2f(0.f, 0.f); gxVertex2f(0.f, 0.f);
						gxTexCoord2f(1.f, 0.f); gxVertex2f(sx,  0.f);
						gxTexCoord2f(1.f, 1.f); gxVertex2f(sx,  sy );
						gxTexCoord2f(0.f, 1.f); gxVertex2f(0.f, sy );
					}
					gxEnd();
					gxSetTexture(0);
				}
				popSurface();
			}
		}
		
		if (mediaPlayer->context->hasBegun)
			mediaPlayer->presentTime += dt;
		//else
		//	logDebug("??");
	}
	
	if (mediaPlayer2)
	{
		mediaPlayer2->tick(mediaPlayer2->context);
	}
}

GLuint VideoLoop::getTexture() const
{
	return mediaPlayer ? mediaPlayer->getTexture() : 0;
}

GLuint VideoLoop::getFirstFrameTexture() const
{
	return firstFrame ? firstFrame->getTexture() : 0;
}

bool VideoLoop::getVideoProperties(int & sx, int & sy, double & duration) const
{
	if (mediaPlayer)
	{
		return mediaPlayer->getVideoProperties(sx, sy, duration);
	}
	else
	{
		sx = 0;
		sy = 0;
		duration = 0.f;
		
		return false;
	}
}
