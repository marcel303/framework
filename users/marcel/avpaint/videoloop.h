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
			mediaPlayer->tick(mediaPlayer->context, true);
			
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
			mediaPlayer2->tick(mediaPlayer2->context, true);
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
