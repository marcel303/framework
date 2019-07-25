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
	MediaPlayer * mediaPlayer1;
	MediaPlayer * mediaPlayer2;
	
	VideoLoop(const char * _filename)
		: filename()
		, mediaPlayer1(nullptr)
		, mediaPlayer2(nullptr)
	{
		filename = _filename;
		
		mediaPlayer1 = new MediaPlayer();
		mediaPlayer1->openAsync(filename.c_str(), MP::kOutputMode_RGBA);
		
		mediaPlayer2 = new MediaPlayer();
		mediaPlayer2->openAsync(filename.c_str(), MP::kOutputMode_RGBA);
	}
	
	~VideoLoop()
	{
		delete mediaPlayer1;
		mediaPlayer1 = nullptr;
		
		delete mediaPlayer2;
		mediaPlayer2 = nullptr;
	}
	
	void tick(const float targetTime, const float dt)
	{
		if (mediaPlayer1->context->hasPresentedLastFrame)
		{
			//switchVideos();
		}
		
		mediaPlayer1->presentTime = targetTime;
		mediaPlayer1->tick(mediaPlayer1->context, true);
		
		mediaPlayer2->tick(mediaPlayer2->context, true);
	}
	
	void seekTo(const float targetTime, const bool nearest = true)
	{
		if (mediaPlayer1 != nullptr)
			mediaPlayer1->seek(targetTime, nearest);
	}
	
	void switchVideos()
	{
		std::swap(mediaPlayer1, mediaPlayer2);
		
		mediaPlayer2->close(false);
		mediaPlayer2->presentTime = 0.0;
		mediaPlayer2->openAsync(filename.c_str(), MP::kOutputMode_RGBA);
	}
	
	GxTextureId getTexture() const
	{
		return mediaPlayer1->getTexture();
	}
};
