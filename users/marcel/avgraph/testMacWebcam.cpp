#ifdef MACOS

#include "framework.h"
#include "vfxNodes/macWebcam.h"
#include "vfxNodes/openglTexture.h"

extern const int GFX_SX;
extern const int GFX_SY;

void testMacWebcam()
{
	MacWebcam * webcam = new MacWebcam();
	
	if (webcam->init() == false)
	{
		logDebug("webcam init failed");
		
		delete webcam;
		webcam = nullptr;
		
		return;
	}

	OpenglTexture texture;
	
	int lastImageIndex = -1;
	
	do
	{
		framework.process();
		
		//
		
		webcam->tick();
		
		if (webcam->image && webcam->image->index != lastImageIndex)
		{
			if (texture.isChanged(webcam->image->sx, webcam->image->sy, GL_RGBA8))
			{
				texture.allocate(webcam->image->sx, webcam->image->sy, GL_RGBA8, false, true);
			}
			
			texture.upload(webcam->image->data, 4, webcam->image->pitch / 4, GL_RGBA, GL_UNSIGNED_BYTE);
		}
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			if (texture.id != 0)
			{
				setColor(colorWhite);
				gxPushMatrix();
				gxTranslatef(0, 0, 0);
				gxSetTexture(texture.id);
				drawRect(0, 0, texture.sx, texture.sy);
				gxSetTexture(0);
				gxPopMatrix();
			}
			
			gxPushMatrix();
			{
				const float scale = std::cos(framework.time * .1f);
				//const float scale = 1.f;
				
				gxTranslatef(100, 100, 0);
				gxRotatef(framework.time * 10, 0, 0, 1);
				gxScalef(scale, scale, 1);
				gxTranslatef(-30, -30, 0);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				{
					const int border = 8;
					
					setColor(colorWhite);
					hqFillRoundedRect(-border, -border, 100+border, 100+border, 10+border);
					
					setColor(200, 200, 255);
					hqFillRoundedRect(0, 0, 100, 100, 10);
				}
				hqEnd();
			}
			gxPopMatrix();
			
			setFont("calibri.ttf");
			pushFontMode(FONT_SDF);
			setColor(colorGreen);
			drawText(GFX_SX/2, GFX_SY/2, 20, 0, 0, "webcam image index: %d", webcam->image ? webcam->image->index : -1);
			drawText(GFX_SX/2, GFX_SY/2 + 30, 20, 0, 0, "conversion time: %.2fms", webcam->context->conversionTimeUsAvg / 1000.0);
			popFontMode();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
	
	webcam->shut();
	
	delete webcam;
	webcam = nullptr;
}

#else

void testMacWebcam()
{
}

#endif
