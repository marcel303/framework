#include "framework.h"

/*
Coding Challenge #116 Continued: Lissajous Curve Table in p5.js
https://www.youtube.com/watch?v=glDU8Nsyidg
*/

static void drawLissajous(const float freqX, const float freqY, const int numSegments, const float phase)
{
	gxPushMatrix();
	gxTranslatef(50, 50, 0);
	gxScalef(40, 40, 1);
	
	pushBlend(BLEND_MAX);
	pushColorPost(POST_PREMULTIPLY_RGB_WITH_ALPHA);
	
	hqBegin(HQ_LINES, true);
	{
		const float x = cosf((float(M_PI) * 2.f * phase * freqX));
		const float y = sinf((float(M_PI) * 2.f * phase * freqY));
		
		setColor(colorWhite);
		hqLine(x, -1.f, .5f, x, +1.f, .5f);
		hqLine(-1.f, y, .5f, +1.f, y, .5f);
	}
	hqEnd();
	
	hqBegin(HQ_LINES, true);
	{
		for (int i = 0; i < numSegments; ++i)
		{
			const float t1 = (i + 0) / float(numSegments);
			const float t2 = (i + 1) / float(numSegments);
			
			const float x1 = cosf((float(M_PI) * 2.f * t1 * freqX));
			const float y1 = sinf((float(M_PI) * 2.f * t1 * freqY));
			
			const float x2 = cosf((float(M_PI) * 2.f * t2 * freqX));
			const float y2 = sinf((float(M_PI) * 2.f * t2 * freqY));
			
			setColor(t1 < phase ? colorRed : colorYellow);
			hqLine(x1, y1, 1.4f, x2, y2, 1.4f);
		}
	}
	hqEnd();
	
	hqBegin(HQ_FILLED_CIRCLES, true);
	{
		const float x = cosf((float(M_PI) * 2.f * phase * freqX));
		const float y = sinf((float(M_PI) * 2.f * phase * freqY));
		
		setColor(colorWhite);
		hqFillCircle(x, y, 4.f);
	}
	hqEnd();
	
	popColorPost();
	popBlend();
	
	gxPopMatrix();
}

int main(int argc, char * argv[])
{
	if (!framework.init(1200, 800))
		return -1;

	while (!framework.quitRequested)
	{
		framework.process();

		framework.beginDraw(0, 0, 0, 0);
		{
			const float phase = fmodf(framework.time * .1f, 1.f);
			
			for (int x = 0; x < 12; ++x)
			{
				for (int y = 0; y < 8; ++y)
				{
					gxPushMatrix();
					{
						gxTranslatef(x * 100, y * 100, 0);
						setDrawRect(x * 100, y * 100, (x + 1) * 100, (y + 1) * 100);
						drawLissajous(x + 1, y + 1, 1000, phase);
						clearDrawRect();
					}
					gxPopMatrix();
				}
			}
		}
		framework.endDraw();
	}
}
