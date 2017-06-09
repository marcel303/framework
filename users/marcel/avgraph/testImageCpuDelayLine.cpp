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

void testImageCpuDelayLine()
{
	const char * videoFilename = "mocapc.mp4";
	const int saveBufferSize = 1024 * 1024;
	
	ImageCpuDelayLine * d = new ImageCpuDelayLine();

	d->init(20, saveBufferSize);

	d->shut();

	d->init(100, saveBufferSize);
	
	MediaPlayer * mediaPlayer = new MediaPlayer();
	mediaPlayer->openAsync(videoFilename, MP::kOutputMode_PlanarYUV);
	
	OpenglTexture texture;
	
	do
	{
		framework.process();
		
		//
		
		mediaPlayer->presentTime += framework.timeStep;
		
		mediaPlayer->tick(mediaPlayer->context, true);
		
		if (mediaPlayer->presentedLastFrame(mediaPlayer->context))
		{
			mediaPlayer->close(false);
			
			mediaPlayer->presentTime = 0.0;
			
			mediaPlayer->openAsync(videoFilename, MP::kOutputMode_PlanarYUV);
		}
		
		//
		
		VfxImageCpu image;
		
		if (mediaPlayer->videoFrame)
		{
			int sx;
			int sy;
			int pitch;
			
			const uint8_t * buffer = mediaPlayer->videoFrame->getY(sx, sy, pitch);
		
			image.setDataR8(buffer, sx, sy, 1, pitch);
			
			//
			
			d->add(image);
		}
		
		//
		
		VfxImageCpu * delayedImage = d->get(d->maxHistorySize - 1);
		
		if (delayedImage == nullptr)
		{
			logDebug("delayedImage is NULL");
		}
		else
		{
			logDebug("delayedImage: sx=%d, sy=%d", delayedImage->sx, delayedImage->sy);
			
			for (int i = 0; i < 100; ++i)
				((uint8_t*)delayedImage->channel[0].data)[delayedImage->channel[0].pitch * (rand() % delayedImage->sy) + (rand() % (delayedImage->sx * 4))] = rand();
		}
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			gxPushMatrix();
			{
				int x = 0;
				int y = 100;
				
				if (mediaPlayer->videoFrame != nullptr)
				{
					gxSetTexture(mediaPlayer->getTexture());
					drawRect(x, y, x + mediaPlayer->videoFrame->m_width, y + mediaPlayer->videoFrame->m_height);
					gxSetTexture(0);
					
					x += mediaPlayer->videoFrame->m_width;
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
					drawRect(x, y, x + delayedImage->sx, y + delayedImage->sy);
					gxSetTexture(0);
					
					x += delayedImage->sx;
				}
			}
			gxPopMatrix();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	delete mediaPlayer;
	mediaPlayer = nullptr;
	
	d->shut();

	delete d;
	d = nullptr;
}
