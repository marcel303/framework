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
#include "objects/blobDetector.h"
#include "Timer.h"
#include "vfxNodes/openglTexture.h"
#include <cmath>

extern const int GFX_SX;
extern const int GFX_SY;

#define SURFACE_SX 320
#define SURFACE_SY 240

#if ENABLE_BLOBDETECTOR_STATS
extern int maxRecursionLevel;
#endif

void testBlobDetection()
{
	Surface surface(SURFACE_SX, SURFACE_SY, false);
	
	// draw some circles in a surface and read back the pixel data into an array that we'll use for detecting blobs

	uint8_t rgba[SURFACE_SX * SURFACE_SY * 4];
	
	pushSurface(&surface);
	{
		surface.clear();

		pushBlend(BLEND_ALPHA);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < 50; ++i)
				{
					const float x = rand() % surface.getWidth();
					const float y = rand() % surface.getHeight();
					
					setColor(colorWhite);
					hqFillCircle(x, y, 10.f);
				}
			}
			hqEnd();
		}
		popBlend();
		
		//
		
		glReadPixels(0, 0, SURFACE_SX, SURFACE_SY, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
		checkErrorGL();
	}
	popSurface();
	
	//for (int i = 0; i < 10000; ++i) // useful when profiling
	for (int i = 0; i < 40; ++i)
	{
		const int kMaxBlobs = 128;
		
		const uint16_t t1 = g_TimerRT.TimeUS_get();
		
		Blob blobs[kMaxBlobs];
		
		uint8_t values[SURFACE_SX * SURFACE_SY];
		BlobDetector::computeValuesFromRGBA(rgba, SURFACE_SX, SURFACE_SY, 0, values);
		
	#if ENABLE_BLOBDETECTOR_STATS
		maxRecursionLevel = 0;
	#else
		const int maxRecursionLevel = -1;
	#endif
	
		const int numBlobs = BlobDetector::detectBlobs(values, SURFACE_SX, SURFACE_SY, blobs, kMaxBlobs);
		
		const uint16_t t2 = g_TimerRT.TimeUS_get();
		
		printf("blob detection took %.2fms. detected %d blobs. maxRecursionLevel=%d\n", (t2 - t1) / 1000.0, numBlobs, maxRecursionLevel);
	}
	
	//exit(0);
	
	OpenglTexture texture;
	texture.allocate(SURFACE_SX, SURFACE_SY, GL_RGBA8, false, true);
	texture.setSwizzle(GL_RED, GL_RED, GL_RED, GL_ONE);
	
	float time = 0.f;
	
	do
	{
		framework.process();
		
		time += framework.timeStep * .4f; // tweaked it a little so it's easier to see how smooth (or not) the detected blob (x, y)'s are updated
		
	#if 1
		// draw some circles in a surface and read back the pixel data into an array that we'll use for detecting blobs
	
		pushSurface(&surface);
		{
			surface.clear();
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < 40; ++i)
				{
					const float x = (std::sin(i + time / 1.23f * (i + 1) / 100.f) + 1.f) / 2.f * surface.getWidth();
					const float y = (std::sin(i + time / 3.45f * (i + 1) / 100.f) + 1.f) / 2.f * surface.getHeight();
					
					setColor(colorWhite);
					hqFillCircle(x, y, 11.f);
				}
			}
			hqEnd();
			
			//
			
			glReadPixels(0, 0, SURFACE_SX, SURFACE_SY, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
			checkErrorGL();
		}
		popSurface();
	#endif
		
		// detect blobs
		
		const int treshold = 255 * mouse.x / GFX_SX;
		
		const int kMaxBlobs = 128;
		Blob blobs[kMaxBlobs];
		
		uint8_t values[SURFACE_SX * SURFACE_SY];
		BlobDetector::computeValuesFromRGBA(rgba, SURFACE_SX, SURFACE_SY, treshold, values);
		texture.upload(values, 1, SURFACE_SX, GL_RED, GL_UNSIGNED_BYTE);
		
		const int numBlobs = BlobDetector::detectBlobs(values, SURFACE_SX, SURFACE_SY, blobs, kMaxBlobs);
		
		// draw the results!

		framework.beginDraw(0, 0, 0, 0);
		{
			gxSetTexture(surface.getTexture());
			{
				pushBlend(BLEND_OPAQUE);
				setColor(colorWhite);
				drawRect(0, 0, surface.getWidth(), surface.getHeight());
				popBlend();
			}
			gxSetTexture(0);
			
			gxTranslatef(SURFACE_SX, 0, 0);
			gxSetTexture(texture.id);
			{
				pushBlend(BLEND_OPAQUE);
				drawRect(0, 0, surface.getWidth(), surface.getHeight());
				popBlend();
			}
			gxSetTexture(0);
			
			hqBegin(HQ_STROKED_CIRCLES);
			{
				for (int i = 0; i < numBlobs; ++i)
				{
					auto & blob = blobs[i];
					
					setColor(colorGreen);
					hqStrokeCircle(blob.x, blob.y, 9.f, 2.f);
				}
			}
			hqEnd();
		}
		framework.endDraw();
	}
	while (!keyboard.wentDown(SDLK_ESCAPE));
}
