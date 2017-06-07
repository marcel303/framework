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
	
	ImageData * image = loadImage("deepbelief/dog.jpg");
	Assert(image);
	
	Deepbelief * d = new Deepbelief();
	
	d->init(networkFilename);
	
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
			if (automaticUpdates)
			{
				d->process((uint8_t*)image->imageData, image->sx, image->sy, 4, image->sx * 4, certaintyTreshold);
			}
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			hqBegin(HQ_FILLED_CIRCLES);
			{
				const float x = GFX_SX/2 + std::cos(framework.time / 1.234f) * 100.f;
				const float y = GFX_SY/2 + std::sin(framework.time / 2.345f) * 100.f;
				
				setColor(100, 100, 100);
				hqFillCircle(x, y, 10.f);
			}
			hqEnd();
			
			setFont("calibri.ttf");
			setColor(colorGreen);
			
			drawText(20, 20, 14, +1, +1, "buffer creation took %.2fms", result.bufferCreationTime);
			drawText(20, 40, 14, +1, +1, "classification took %.2fms", result.classificationTime);
			drawText(20, 60, 14, +1, +1, "sorting took %.2fms", result.sortTime);
			
			int index = 0;
			
			for (auto & p : result.predictions)
			{
				drawText(20, 100 + index * 30, 14, +1, +1, "%d: %s @ %.3f certainty", index, p.label.c_str(), p.certainty);
				
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
