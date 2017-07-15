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
#include "stb_truetype.h"
#include "testBase.h"

static stbtt_bakedchar cdata[96]; // ASCII 32..126 is 95 glyphs

static void stbTruetype_Print(float x, float y, const char *text)
{
	// assume orthographic projection with units = screen pixels, origin at top left
	gxBegin(GL_QUADS);
	{
		while (*text)
		{
			if (*text >= 32 && *text < 128)
			{
				stbtt_aligned_quad q;
				stbtt_GetBakedQuad(cdata, 512,512, *text-32, &x,&y,&q,1);//1=opengl & d3d10+,0=d3d9
				
				gxTexCoord2f(q.s0,q.t1); gxVertex2f(q.x0,q.y1);
				gxTexCoord2f(q.s1,q.t1); gxVertex2f(q.x1,q.y1);
				gxTexCoord2f(q.s1,q.t0); gxVertex2f(q.x1,q.y0);
				gxTexCoord2f(q.s0,q.t0); gxVertex2f(q.x0,q.y0);
			}
			
			++text;
		}
	}
	gxEnd();
}

void testStbTruetype()
{
	static unsigned char ttf_buffer[1<<20];
	static unsigned char temp_bitmap[512*512];

	fread(ttf_buffer, 1, 1<<20, fopen("calibri.ttf", "rb"));
	stbtt_BakeFontBitmap(ttf_buffer, 0, 32.0, temp_bitmap, 512, 512, 32, 96, cdata); // no guarantee this fits!

	// can free ttf_buffer at this point
	GLuint fontTexture = createTextureFromR8(temp_bitmap, 512, 512, false, true);
	
	Path2d path;
	
	do
	{
		framework.process();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			auto myDrawRect = [](float x1, float y1, float x2, float y2)
			{
				gxBegin(GL_QUADS);
				{
					gxTexCoord2f(0.f, 0.f); gxVertex2f(x1, y1);
					gxTexCoord2f(1.f, 0.f); gxVertex2f(x2, y1);
					gxTexCoord2f(1.f, 1.f); gxVertex2f(x2, y2);
					gxTexCoord2f(0.f, 1.f); gxVertex2f(x1, y2);
				}
				gxEnd();
			};
			
			gxSetTexture(fontTexture);
			{
				setColor(colorWhite);
				myDrawRect(0, 0, 512*2, 512*2);
			
				setColor(colorWhite);
				stbTruetype_Print(300, 300, "Hello World!");
			}
			gxSetTexture(0);
			
			gxPushMatrix();
			{
				gxTranslatef(400, 400, 0);
				gxScalef(.1f, .1f, 1.f);
				setColor(colorWhite);
				drawPath(path);
				//hqDrawPath(path);
			}
			gxPopMatrix();
			
			drawTestUi();
		}
		framework.endDraw();
	}
	while (tickTestUi());
}
