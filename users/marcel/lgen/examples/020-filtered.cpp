#include "framework.h"
#include "Lgen.h"

static GxTextureId createTexture(const lgen::Heighfield & heightfield)
{
	uint8_t * values = new uint8_t[heightfield.w * heightfield.h];
	
	for (int y = 0; y < heightfield.h; ++y)
		for (int x = 0; x < heightfield.w; ++x)
			values[x + y * heightfield.w] = heightfield.getHeight(x, y);
	
	GxTextureId result = createTextureFromR8(values, heightfield.w, heightfield.h, false, false);
	
	delete [] values;
	
	return result;
}

int main(int argc, char * argv[])
{
	if (!framework.init(512, 512))
		return -1;
	
	lgen::Heighfield heightfield;
	heightfield.setSize(512, 512);
	
	lgen::Generator_OffsetSquare generator;
	generator.generate(heightfield);
	
	lgen::FilterMean filter;
	filter.setMatrixSize(32, 32);
	filter.apply(heightfield, heightfield);
	
	heightfield.rerange(0, 255);
	
	GxTextureId texture = createTexture(heightfield);
	
	bool repeat = false;
	
	for (;;)
	{
		framework.waitForEvents = true;
		framework.process();
		
		if (framework.quitRequested)
			break;
		
		if (keyboard.wentDown(SDLK_r))
			repeat = !repeat;
		
		if (mouse.wentDown(BUTTON_LEFT))
		{
			generator.generate(heightfield);
			filter.apply(heightfield, heightfield);
			heightfield.rerange(0, 255);
			
			freeTexture(texture);
			texture = createTexture(heightfield);
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			gxSetTexture(texture);
			gxBegin(GX_QUADS);
			{
				const float x1 = 0;
				const float y1 = 0;
				const float x2 = 512;
				const float y2 = 512;
				const float s = repeat ? 4 : 1;
				gxTexCoord2f(0, 0); gxVertex2f(x1, y1);
				gxTexCoord2f(s, 0); gxVertex2f(x2, y1);
				gxTexCoord2f(s, s); gxVertex2f(x2, y2);
				gxTexCoord2f(0, s); gxVertex2f(x1, y2);
			}
			gxEnd();
			gxSetTexture(0);
		}
		framework.endDraw();
	}
	
	freeTexture(texture);
	
	framework.shutdown();
	
	return 0;
}
