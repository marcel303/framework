#include "framework.h"
#include "image.h"
#include "Timer.h"
#include "vfxNodes/deepbelief.h"
#include "video.h"
#include <DeepBelief/DeepBelief.h>

extern const int GFX_SX;
extern const int GFX_SY;

void testDeepbelief()
{
	const char * networkFilename = "deepbelief/jetpac.ntwk";
	const char * imageFilename = "deepbelief/dog.jpg";
	//const char * imageFilename = "deepbelief/rainbow.png"; // apparently it looks like a banana! :-)
	ImageData * image = loadImage(imageFilename);
	Assert(image);
	
	Deepbelief * d = new Deepbelief();
	
	d->init(networkFilename);
	
	d->process((uint8_t*)image->imageData, image->sx, image->sy, 4, image->sx * 4, certaintyTreshold);
	
	DeepbeliefResult result;
	
	bool automaticUpdates = false;
	
	float certaintyTreshold = .01f;
	
	do
	{
		framework.process();
		
		if (keyboard.wentDown(SDLK_p))
		{
			d->process((uint8_t*)image->imageData, image->sx, image->sy, 4, image->sx * 4, certaintyTreshold);
		}
		
		if (keyboard.wentDown(SDLK_w))
		{
			d->process((uint8_t*)image->imageData, image->sx, image->sy, 4, image->sx * 4, certaintyTreshold);
			d->wait();
		}
		
		if (keyboard.isDown(SDLK_r))
		{
			d->process((uint8_t*)image->imageData, image->sx, image->sy, 4, image->sx * 4, certaintyTreshold);
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
		
		if (d->getResult(result))
		{
			//
		}
		
		if (automaticUpdates)
		{
			d->process((uint8_t*)image->imageData, image->sx, image->sy, 4, image->sx * 4, certaintyTreshold);
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			const GLuint texture = getTexture(imageFilename);
			
			gxPushMatrix();
			{
				gxTranslatef(GFX_SX-200, GFX_SY-200, 0);
				gxSetTexture(texture);
				setColor(colorWhite);
				drawRect(0, 0, 150, 150);
				gxSetTexture(0);
				setColor(colorGreen);
				drawRectLine(0, 0, 150, 150);
			}
			gxPopMatrix();
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < 100; ++i)
				{
					const float x = GFX_SX/2 + std::cos(i / 30.f * framework.time / 1.234f) * 200.f;
					const float y = GFX_SY/2 + std::sin(i / 30.f * framework.time / 2.345f) * 100.f;
					const float a = .5f + .5f * std::cos(i / 100.f * framework.time / 4.567f);
					
					setColorf(.4f, .4f, .4f, a);
					hqFillCircle(x, y, 10.f);
				}
			}
			hqEnd();
			
			setFont("calibri.ttf");
			setColor(colorGreen);
			
			drawText(20, 20, 14, +1, +1, "initialized: %d, automaticProcessing: %d", d->isInitialized, automaticUpdates);
			drawText(20, 60, 14, +1, +1, "buffer creation took %.2fms", result.bufferCreationTime);
			drawText(20, 80, 14, +1, +1, "classification took %.2fms", result.classificationTime);
			drawText(20, 100, 14, +1, +1, "sorting took %.2fms", result.sortTime);
			
			drawText(GFX_SX/2, 20, 14, +1, +1, "P: do single classification (async). notice how the animation remains smooth");
			drawText(GFX_SX/2, 40, 14, +1, +1, "W: do single classification (wait). notice the animation hitches");
			drawText(GFX_SX/2, 60, 14, +1, +1, "I: initialize deep belief object");
			drawText(GFX_SX/2, 80, 14, +1, +1, "S: shut down deep belief object");
			drawText(GFX_SX/2, 100, 14, +1, +1, "A: toggle automatic processing");
			drawText(GFX_SX/2, 120, 14, +1, +1, "R: process (continuously)");
			
			int index = 0;
			
			for (auto & p : result.predictions)
			{
				setColorf(p.certainty, p.certainty + .5f, p.certainty);
				drawText(40, 140 + index * 20, 14, +1, +1, "%d: %s @ %.3f certainty", index, p.label.c_str(), p.certainty);
				
				index++;
			}
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	d->shut();
	
	delete d;
	d = nullptr;
	
	delete image;
	image = nullptr;
}
