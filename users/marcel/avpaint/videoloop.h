#pragma once

#include "framework.h"
#include "video.h"

struct VideoLoop
{
	std::string filename;
	MediaPlayer * mediaPlayer;
	MediaPlayer * mediaPlayer2;
	Surface * firstFrame;
	
	VideoLoop(const char * _filename)
		: filename()
		, mediaPlayer(nullptr)
		, mediaPlayer2(nullptr)
		, firstFrame(nullptr)
	{
		filename = _filename;
	}
	
	~VideoLoop()
	{
		delete firstFrame;
		firstFrame = nullptr;
		
		delete mediaPlayer;
		mediaPlayer = nullptr;
		
		delete mediaPlayer2;
		mediaPlayer2 = nullptr;
	}
	
	void tick(const float dt)
	{
		if (mediaPlayer2 == nullptr)
		{
			if (mediaPlayer == nullptr || mediaPlayer->presentTime >= 1.0)
			{
				mediaPlayer2 = new MediaPlayer();
				mediaPlayer2->openAsync(filename.c_str(), MP::kOutputMode_RGBA);
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
	
	GLuint getTexture() const
	{
		return mediaPlayer->getTexture();
	}
	
	GLuint getFirstFrameTexture() const
	{
		return firstFrame ? firstFrame->getTexture() : 0;
	}
};
