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

#ifdef MACOS

#include "framework.h"
#include "image.h"
#include "Timer.h"
#include "vfxNodes/deepbelief.h"
#include "../avpaint/video.h"
#include "mediaplayer_new/MPVideoBuffer.h"

extern const int GFX_SX;
extern const int GFX_SY;

void testDeepbelief()
{
	const char * networkFilename = "deepbelief/jetpac.ntwk";
	//const char * networkFilename = "deepbelief/ccv2010.ntwk";
	//const char * imageFilename = "deepbelief/dog.jpg";
	//const char * imageFilename = "deepbelief/rainbow.png"; // apparently it looks like a banana! :-)
	const char * imageFilename = "deepbelief/tea.jpg";
	//const char * videoFilename = "mocapb.mp4";
	const char * videoFilename = "deepbelief/objects1.mp4";
	
	ImageData * image = loadImage(imageFilename);
	Assert(image);
	
	auto videoOutputMode = MP::kOutputMode_RGBA;
	//auto videoOutputMode = MP::kOutputMode_PlanarYUV;
	
	MediaPlayer mediaPlayer;
	mediaPlayer.openAsync(videoFilename, videoOutputMode);
	
	Deepbelief * d = new Deepbelief();
	
	d->init("");
	
	d->shut();
	
	d->init(networkFilename);
	
	DeepbeliefResult result;
	
	bool automaticUpdates = true;
	
	bool sampleVideo = true;
	
	float certaintyTreshold = .01f;
	
	bool isFirstFrame = true;
	
	do
	{
		framework.process();
		
		// the specifications of the image we want to process
		const uint8_t * buffer;
		int sx;
		int sy;
		int numChannels;
		int pitch;
		
		// set the image specifications to a 1x1 single channel black image initially
		uint8_t black = 0;
		buffer = &black;
		sx = 1;
		sy = 1;
		numChannels = 1;
		pitch = 1;
		
		if (sampleVideo)
		{
			if (mediaPlayer.videoFrame != nullptr)
			{
				if (videoOutputMode == MP::kOutputMode_RGBA)
				{
					buffer = mediaPlayer.videoFrame->m_frameBuffer;
					sx = mediaPlayer.videoFrame->m_width;
					sy = mediaPlayer.videoFrame->m_height;
					numChannels = 4;
					
					const int alignment = 16;
					const int alignmentMask = ~(alignment - 1);
					
					pitch = (((sx * 4) + alignment - 1) & alignmentMask);
				}
				else
				{
					buffer = mediaPlayer.videoFrame->getY(sx, sy, pitch);
					numChannels = 1;
				}
			}
		}
		else
		{
			buffer = (uint8_t*)image->imageData;
			sx = image->sx;
			sy = image->sy;
			numChannels = 4;
			pitch= image->sx * 4;
		}
		
		if (isFirstFrame)
		{
			isFirstFrame = false;
			
			d->process(buffer, sx, sy, numChannels, pitch, certaintyTreshold);
		}
		
		if (keyboard.wentDown(SDLK_p))
		{
			d->process(buffer, sx, sy, numChannels, pitch, certaintyTreshold);
		}
		
		if (keyboard.wentDown(SDLK_w))
		{
			d->process(buffer, sx, sy, numChannels, pitch, certaintyTreshold);
			d->wait();
		}
		
		if (keyboard.isDown(SDLK_r))
		{
			d->process(buffer, sx, sy, numChannels, pitch, certaintyTreshold);
		}
		
		if (keyboard.wentDown(SDLK_a))
		{
			automaticUpdates = !automaticUpdates;
		}
		
		if (keyboard.wentDown(SDLK_i))
		{
			d->init(networkFilename);
		}
		if (keyboard.wentDown(SDLK_s))
		{
			d->shut();
		}
		
		if (keyboard.wentDown(SDLK_v))
		{
			sampleVideo = !sampleVideo;
		}
		
		if (d->getResult(result))
		{
			//
		}
		
		if (automaticUpdates)
		{
			d->process(buffer, sx, sy, numChannels, pitch, certaintyTreshold);
		}
		
		mediaPlayer.presentTime += framework.timeStep;
		
		mediaPlayer.tick(mediaPlayer.context, true);
		
		if (mediaPlayer.presentedLastFrame(mediaPlayer.context))
		{
			mediaPlayer.close(false);
			
			mediaPlayer.presentTime = 0.0;
			
			mediaPlayer.openAsync(videoFilename, videoOutputMode);
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			const GLuint texture = sampleVideo ? mediaPlayer.getTexture() : getTexture(imageFilename);
			
			const int size = 400;
			const int padding = (GFX_SX - size * 2) / 3;
			
			gxPushMatrix();
			{
				gxTranslatef(GFX_SX-size-padding, GFX_SY-size-padding, 0);
				gxSetTexture(texture);
				setColor(colorWhite);
				drawRect(0, 0, size, size);
				gxSetTexture(0);
				setColor(colorGreen);
				drawRectLine(0, 0, size, size);
			}
			gxPopMatrix();
			
			gxPushMatrix();
			{
				gxTranslatef(padding, GFX_SY-size-padding, 0);
				
				hqBegin(HQ_FILLED_CIRCLES);
				{
					for (int i = 0; i < 100; ++i)
					{
						const float x = size/2 + std::cos(i / 30.f * framework.time / 1.234f) * size * 4/10.f;
						const float y = size/2 + std::sin(i / 30.f * framework.time / 2.345f) * size * 2/10.f;
						const float a = .5f + .5f * std::cos(i / 100.f * framework.time / 4.567f);
						
						setColorf(.4f, .4f, .4f, a);
						hqFillCircle(x, y, 10.f);
					}
				}
				hqEnd();
			}
			gxPopMatrix();
			
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			setColor(colorGreen);
			
			drawText(20, 20, 14, +1, +1, "initialized: %d, automaticProcessing: %d", d->state && d->state->isInitialized, automaticUpdates);
			drawText(20, 60, 14, +1, +1, "buffer creation took %.2fms", result.bufferCreationTime);
			drawText(20, 80, 14, +1, +1, "classification took %.2fms", result.classificationTime);
			drawText(20, 100, 14, +1, +1, "sorting took %.2fms", result.sortTime);
			
			drawText(GFX_SX/2, 20, 14, +1, +1, "P: do single classification (async). notice how the animation remains smooth");
			drawText(GFX_SX/2, 40, 14, +1, +1, "W: do single classification (wait). notice the animation hitches");
			drawText(GFX_SX/2, 60, 14, +1, +1, "I: initialize deep belief object");
			drawText(GFX_SX/2, 80, 14, +1, +1, "S: shut down deep belief object");
			drawText(GFX_SX/2, 100, 14, +1, +1, "A: toggle automatic processing (%s)", automaticUpdates ? "on" : "off");
			drawText(GFX_SX/2, 120, 14, +1, +1, "R: process (continuously)");
			popFontMode();
			
			int index = 0;
			
			for (auto & p : result.predictions)
			{
				if (index <= 9)
				{
					setColorf(p.certainty, p.certainty + .5f, p.certainty);
					drawText(40, 140 + index * 20, 14, +1, +1, "%d: %s @ %.2f%% certainty", index + 1, p.label.c_str(), p.certainty * 100.f);
				}
					
				index++;
			}
			
			if (!result.predictions.empty())
			{
				gxPushMatrix();
				gxTranslatef(GFX_SX*4/9, 220, 0);
				const float scale = 1.f + std::sin(framework.time / 1.234f) * .2f;
				gxScalef(scale, scale, 1);
				gxRotatef(std::sin(framework.time / 2.345f) * 5.f, 0, 0, 1);
				pushFontMode(FONT_SDF);
				setColor(200, 200, 200);
				drawText(0, 0, 48, 0, 0, "%s", result.predictions.front().label.c_str());
				popFontMode();
				gxPopMatrix();
			}
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	d->shut();
	
	delete d;
	d = nullptr;
	
	mediaPlayer.close(true);
	
	delete image;
	image = nullptr;
}

#endif
