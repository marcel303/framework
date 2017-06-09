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

#include "framework.h"
#include "vfxNodes/imageCpuDelayLine.h"
#include "vfxNodes/openglTexture.h"
#include "vfxNodes/vfxNodeBase.h"

#include "../avpaint/video.h"
#include "mediaplayer_new/MPVideoBuffer.h"

extern const int GFX_SX;
extern const int GFX_SY;

void testImageCpuDelayLine()
{
	const char * videoFilename = "mocapc.mp4";
	const int saveBufferSize = 256 * 1024;
	
	ImageCpuDelayLine * d = new ImageCpuDelayLine();

	d->init(20, saveBufferSize);

	d->shut();

	d->init(240, saveBufferSize);
	
	MediaPlayer * mediaPlayer = new MediaPlayer();
	mediaPlayer->openAsync(videoFilename, MP::kOutputMode_PlanarYUV);
	
	OpenglTexture texture;
	
	float offset = .5f;
	
	do
	{
		framework.process();
		
		SDL_Delay(1);
		
		//
		
		if (keyboard.isDown(SDLK_LEFT))
			offset -= .2f * framework.timeStep;
		if (keyboard.isDown(SDLK_RIGHT))
			offset += .2f * framework.timeStep;
		
		offset = std::max(0.f, std::min(1.f, offset));
		
		if (keyboard.wentDown(SDLK_c))
			d->clearHistory();
		
		if (keyboard.wentDown(SDLK_r))
		{
			d->shut();
			
			d->init(random(10, 240), saveBufferSize);
		}
		
		//
		
		mediaPlayer->presentTime += framework.timeStep;
		
		bool gotVideoFrame = mediaPlayer->tick(mediaPlayer->context, true);
		
		if (mediaPlayer->presentedLastFrame(mediaPlayer->context))
		{
			mediaPlayer->close(false);
			
			mediaPlayer->presentTime = 0.0;
			
			mediaPlayer->openAsync(videoFilename, MP::kOutputMode_PlanarYUV);
			
			// frame we got before is no longer valid, as we re-opened the media player
			gotVideoFrame = false;
		}
		
		//
		
		d->tick();
		
		//
		
		double delayedTimestamp = 0.0;
		
		VfxImageCpu * delayedImage = d->get(int(offset * (d->maxHistorySize - 1)), &delayedTimestamp);
		
		//
		
		const int jpegQualityLevel = (std::cos(framework.time / 2.f) + 1.f) / 2.f * 100.f;
		
		VfxImageCpu image;
		
		if (gotVideoFrame)
		{
			Assert(mediaPlayer->videoFrame != nullptr);
			
			int sx;
			int sy;
			int pitch;
			
			const uint8_t * buffer = mediaPlayer->videoFrame->getY(sx, sy, pitch);
		
			image.setDataR8(buffer, sx, sy, 1, pitch);
			
			//
			
			d->add(image, jpegQualityLevel, mediaPlayer->videoFrame->m_time);
		}
		
	#if 0
		if (delayedImage == nullptr)
		{
			logDebug("delayedImage is NULL");
		}
		else
		{
			logDebug("delayedImage: sx=%d, sy=%d", delayedImage->sx, delayedImage->sy);
		}
	#endif
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			int x = 10;
			int y = 100;
			
			{
				int sy = 0;
				
				if (mediaPlayer->videoFrame != nullptr)
				{
					gxSetTexture(mediaPlayer->getTexture());
					setColor(colorWhite);
					drawRect(x, y, x + mediaPlayer->videoFrame->m_width, y + mediaPlayer->videoFrame->m_height);
					gxSetTexture(0);
					
					x += mediaPlayer->videoFrame->m_width;
					
					sy = std::max(sy, (int)mediaPlayer->videoFrame->m_height);
				}
				
				if (delayedImage != nullptr)
				{
					if (texture.isChanged(delayedImage->sx, delayedImage->sy, GL_RGBA8))
					{
						texture.allocate(delayedImage->sx, delayedImage->sy, GL_RGBA8, true, true);
					}
					
					texture.upload(delayedImage->channel[0].data, 4, delayedImage->channel[0].pitch / 4, GL_RGBA, GL_UNSIGNED_BYTE);
					
					//
					
					gxSetTexture(texture.id);
					setColor(colorWhite);
					drawRect(x, y, x + delayedImage->sx, y + delayedImage->sy);
					gxSetTexture(0);
					
					x += delayedImage->sx;
					
					sy = std::max(sy, delayedImage->sy);
				}
				
				y += sy;
				y += 10;
				
				x = 10;
			}
			
			setFont("calibri.ttf");
			setFontMSDF("calibri.ttf");
			setColor(colorGreen);
			ImageCpuDelayLine::MemoryUsage memoryUsage = d->getMemoryUsage();
			drawText(10, 10, 14, +1, +1, "memory usage: %.2f Mb", memoryUsage.numBytes / 1024.0 / 1024.0);
			drawText(10, 30, 14, +1, +1, "history: %.2f Mb", memoryUsage.numHistoryBytes / 1024.0 / 1024.0);
			drawText(10, 50, 14, +1, +1, "cached image: %.2f Mb", memoryUsage.numCachedImageBytes / 1024.0 / 1024.0);
			drawText(10, 70, 14, +1, +1, "save buffers: %.2f Mb", memoryUsage.numSaveBufferBytes / 1024.0 / 1024.0);
			
			drawText(210, 10, 14, +1, +1, "current jpeg quality level: %d", jpegQualityLevel);
			drawText(210, 30, 14, +1, +1, "history size: %d / %d", memoryUsage.historySize, d->maxHistorySize);
			if (mediaPlayer->videoFrame != nullptr)
				drawText(210, 50, 14, +1, +1, "VIDEO: %d x %d, timestamp: %.2fs", mediaPlayer->videoFrame->m_width, mediaPlayer->videoFrame->m_height, mediaPlayer->videoFrame->m_time);
			if (delayedImage != nullptr)
				drawText(210, 70, 14, +1, +1, "DELAYED IMAGE: %d x %d, timestamp: %.2fs", delayedImage->sx, delayedImage->sy, delayedTimestamp);
			
			setColor(50, 50, 50);
			drawRect(x, y, x + 400 * memoryUsage.historySize / d->maxHistorySize, y + 20);
			setColor(colorWhite);
			drawTextMSDF(x + 5, y + 20/2, 10, +1, 0, "delay line FIFO");
			setColor(100, 100, 100);
			drawRectLine(x, y, x + 400, y + 20);
			
			setColor(colorYellow);
			drawLine(
				x + 400 * offset, y +  0,
				x + 400 * offset, y + 20);
			
			y += 30;
			
			setColor(colorGreen);
			drawText(x, y + 0, 14, +1, +1, "LEFT/RIGHT: control sampling position within the delay line");
			drawText(x, y + 20, 14, +1, +1, "C: clear delay line FIFO");
			drawText(x, y + 40, 14, +1, +1, "R: set a random delay line size");
			drawText(x, y + 60, 14, +1, +1, "SPACE: quit test");
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	delete mediaPlayer;
	mediaPlayer = nullptr;
	
	d->shut();

	delete d;
	d = nullptr;
}
