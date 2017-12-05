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
#include "testBase.h"
#include "vfxNodeBase.h"
#include "vfxNodes/imageCpuDelayLine.h"
#include "vfxNodes/openglTexture.h"

#include "../libvideo/video.h"
#include "mediaplayer/MPVideoBuffer.h"

extern const int GFX_SX;
extern const int GFX_SY;

void testImageCpuDelayLine()
{
	setAbout("This example shows the usage of the 'image delay line'. The image delay line records a history of N images and allows random access to these images. To conserve memory, the image delay line has optional support for JPEG compression.");
	
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
	
	bool glitchEnabled = false;
	
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
		
		if (keyboard.wentDown(SDLK_g))
			glitchEnabled = !glitchEnabled;
		
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
		
		VfxImageCpuData delayedImage;
		const bool gotDelatedImage = d->get(int(offset * (d->maxHistorySize - 1)), delayedImage, &delayedTimestamp, glitchEnabled);
		
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
		if (gotDelatedImage == false)
		{
			logDebug("delayedImage is NULL");
		}
		else
		{
			logDebug("delayedImage: sx=%d, sy=%d", delayedImage.image->sx, delayedImage.image->sy);
		}
	#endif
		
		//
		
		const Color textColor(0, 0, 255);
		const float fontSize = 15.f;
		
		framework.beginDraw(250, 250, 250, 0);
		{
			int x = 0;
			int y = 100;
			
			{
				int sy = 0;
				
				const int padding = 10;
				
				const float videoSx = mediaPlayer->videoFrame ? mediaPlayer->videoFrame->m_width : gotDelatedImage ? delayedImage.image.sx : 1.f;
				
				const float videoScale = float(GFX_SX - padding * 3) / videoSx / 2.f;
				
				if (mediaPlayer->videoFrame != nullptr)
				{
					x += padding;
					
					gxSetTexture(mediaPlayer->getTexture());
					setColor(colorWhite);
					drawRect(x, y, x + mediaPlayer->videoFrame->m_width * videoScale, y + mediaPlayer->videoFrame->m_height * videoScale);
					gxSetTexture(0);
					
					x += mediaPlayer->videoFrame->m_width * videoScale;
					
					sy = std::max(sy, int(mediaPlayer->videoFrame->m_height * videoScale));
				}
				
				if (gotDelatedImage)
				{
					x += padding;
					
					if (texture.isChanged(delayedImage.image.sx, delayedImage.image.sy, GL_R8))
					{
						texture.allocate(delayedImage.image.sx, delayedImage.image.sy, GL_R8, true, true);
						texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
					}
					
					texture.upload(delayedImage.image.channel[0].data, 1, delayedImage.image.channel[0].pitch / 1, GL_RED, GL_UNSIGNED_BYTE);
					
					//
					
					gxSetTexture(texture.id);
					setColor(colorWhite);
					drawRect(x, y, x + delayedImage.image.sx * videoScale, y + delayedImage.image.sy * videoScale);
					gxSetTexture(0);
					
					x += delayedImage.image.sx * videoScale;
					
					sy = std::max(sy, int(delayedImage.image.sy * videoScale));
				}
				
				y += sy;
				y += 10;
				
				x = 10;
			}
			
			setFont("calibri.ttf");
			setColor(textColor);
			ImageCpuDelayLine::MemoryUsage memoryUsage = d->getMemoryUsage();
			drawText(10, 10, fontSize, +1, +1, "memory usage: %.2f Mb", memoryUsage.numBytes / 1024.0 / 1024.0);
			drawText(10, 30, fontSize, +1, +1, "history: %.2f Mb", memoryUsage.numHistoryBytes / 1024.0 / 1024.0);
			drawText(10, 50, fontSize, +1, +1, "cached image: %.2f Mb", memoryUsage.numCachedImageBytes / 1024.0 / 1024.0);
			drawText(10, 70, fontSize, +1, +1, "save buffers: %.2f Mb", memoryUsage.numSaveBufferBytes / 1024.0 / 1024.0);
			
			drawText(210, 10, fontSize, +1, +1, "current jpeg quality level: %d, glitch enabled: %d", jpegQualityLevel, glitchEnabled);
			drawText(210, 30, fontSize, +1, +1, "history size: %d / %d", memoryUsage.historySize, d->maxHistorySize);
			if (mediaPlayer->videoFrame != nullptr)
				drawText(210, 50, fontSize, +1, +1, "VIDEO: %d x %d, timestamp: %.2fs", mediaPlayer->videoFrame->m_width, mediaPlayer->videoFrame->m_height, mediaPlayer->videoFrame->m_time);
			if (gotDelatedImage)
				drawText(210, 70, fontSize, +1, +1, "DELAYED IMAGE: %d x %d, timestamp: %.2fs", delayedImage.image.sx, delayedImage.image.sy, delayedTimestamp);
			
			setColor(150, 150, 250);
			drawRect(x, y, x + 400 * memoryUsage.historySize / d->maxHistorySize, y + 20);
			setColor(colorWhite);
			pushFontMode(FONT_SDF);
			drawText(x + 5, y + 20/2, 10, +1, 0, "delay line FIFO");
			popFontMode();
			setColor(colorRed);
			drawLine(
				x + 400 * offset, y +  0,
				x + 400 * offset, y + 20);
			setColor(50, 50, 150);
			drawRectLine(x, y, x + 400, y + 20);
						
			y += 30;
			
			setColor(textColor);
			drawText(x, y + 0, fontSize, +1, +1, "LEFT/RIGHT: control sampling position within the delay line");
			drawText(x, y + 20, fontSize, +1, +1, "C: clear delay line FIFO");
			drawText(x, y + 40, fontSize, +1, +1, "R: set a random delay line size");
			drawText(x, y + 60, fontSize, +1, +1, "SPACE: quit test");
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
	
	delete mediaPlayer;
	mediaPlayer = nullptr;
	
	d->shut();

	delete d;
	d = nullptr;
}
