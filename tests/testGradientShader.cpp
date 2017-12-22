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
#include "Noise.h"
#include "testBase.h"

extern const int GFX_SX;
extern const int GFX_SY;

void testGradientShader()
{
	setAbout("This test shows how to combine color gradients and texturing with high-quality primitive drawing.");
	
	const int kNumCircles = 32;
	
	Vec3 circles[kNumCircles];
	
	for (int i = 0; i < kNumCircles; ++i)
	{
		circles[i][0] = random(0, GFX_SX);
		circles[i][1] = random(0, GFX_SY);
		circles[i][2] = random(0.f, 5.f); // life
	}
	
	do
	{
		framework.process();
		
		for (int i = 0; i < kNumCircles; ++i)
		{
			circles[i][2] -= framework.timeStep;
			
			if (circles[i][2] <= 0.f)
			{
				circles[i][0] = random(0, GFX_SX);
				circles[i][1] = random(0, GFX_SY);
				circles[i][2] = random(0.f, 5.f); // life
			}
			
			circles[i][0] += scaled_octave_noise_3d(4, .5f, .01f, -500.f, +500.f, framework.time * 20.f + i, circles[i][0], circles[i][1]) * framework.timeStep;
			circles[i][1] += scaled_octave_noise_3d(4, .5f, .01f, -500.f, +500.f, framework.time * 20.f - i, circles[i][0], circles[i][1]) * framework.timeStep;
		}
		
		framework.beginDraw(0, 0, 0, 0);
		{
			const Mat4x4 cmat = Mat4x4(true)
				.Scale(1.f / 100.f, 1.f / 100.f, 1.f)
				.RotateZ(-framework.time)
				.Translate(-GFX_SX/2, -GFX_SY/2, 0);
			const Mat4x4 tmat = Mat4x4(true)
				.Translate(.5f, .5f, 0.f)
				.Scale(1.f / 400.f, 1.f / 400.f, 1.f)
				.RotateZ(+framework.time)
				.Translate(-GFX_SX/2, -GFX_SY/2, 0);
			
			const GRADIENT_TYPE gradientType = mouse.isDown(BUTTON_LEFT) ? GRADIENT_RADIAL : GRADIENT_LINEAR;
			const float gradientBias = mouse.x / float(GFX_SX);
			const float gradientScale = gradientBias == 1.f ? 0.f : 1.f / (1.f - gradientBias);
			hqSetGradient(gradientType, cmat, colorWhite, colorBlack, COLOR_MUL, gradientBias, gradientScale);
			hqSetTexture(tmat, getTexture("happysun.jpg"));
			for (int i = 0; i < 360; i += 20)
			{
				gxPushMatrix();
				gxRotatef(i + framework.time, 0, 0, 1);
				hqBegin(HQ_LINES);
				{
					setColor(colorWhite);
					hqLine(0, 0, 10, GFX_SX, GFX_SY, 50);
				}
				hqEnd();
				gxPopMatrix();
			}
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int i = 0; i < kNumCircles; ++i)
				{
					gxColor4f(1, 1, 1, clamp(circles[i][2], 0.f, 1.f));
					hqFillCircle(circles[i][0], circles[i][1], 20.f);
				}
			}
			hqEnd();
			hqClearGradient();
			hqClearTexture();
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
}
