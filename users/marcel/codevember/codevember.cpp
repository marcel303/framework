#include "Calc.h"
#include "framework.h"
#include "Noise.h"

const int GFX_SX = 800;
const int GFX_SY = 600;

void codevember1()
{
	const int kNumCircles = 32;
	
	Vec3 circles[kNumCircles];
	
	for (int i = 0; i < kNumCircles; ++i)
	{
		circles[i][0] = random(0, GFX_SX);
		circles[i][1] = random(0, GFX_SY);
		circles[i][2] = random(0.f, 10.f); // life
	}
	
	Surface surface(GFX_SX, GFX_SY, true);
	surface.clear();
	
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
			pushSurface(&surface);
			
			const Mat4x4 cmat = Mat4x4(true)
				.Scale(1.f / 300.f, 1.f / 300.f, 1.f)
				.RotateZ(-framework.time * .5f)
				.Translate(-GFX_SX/2, -GFX_SY/2, 0);
			const Mat4x4 tmat = Mat4x4(true)
				.Translate(.5f, .5f, 0.f)
				.Scale(1.f / 400.f, 1.f / 400.f, 1.f)
				.RotateZ(+framework.time)
				.Translate(-GFX_SX/2, -GFX_SY/2, 0);
			
			const GRADIENT_TYPE gradientType = mouse.isDown(BUTTON_LEFT) ? GRADIENT_RADIAL : GRADIENT_LINEAR;
			const float gradientBias = mouse.x / float(GFX_SX);
			const float gradientScale = gradientBias == 1.f ? 0.f : 1.f / (1.f - gradientBias);
			hqSetGradient(gradientType, cmat, Color(1.f, 1.f, 1.f, .1f), Color(0.f, 0.f, 0.f, .1f), COLOR_MUL, gradientBias, gradientScale);
			//hqSetTexture(tmat, getTexture("happysun.jpg"));
			for (int i = 0; i < 360; i += 16)
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
					gxColor4f(1, 1, 1, Calc::Clamp(circles[i][2] / 2.f, 0.f, 1.f));
					hqFillCircle(circles[i][0], circles[i][1], 20.f);
				}
			}
			hqEnd();
			hqClearGradient();
			hqClearTexture();
			
			popSurface();
			surface.blit(BLEND_OPAQUE);
		}
		framework.endDraw();
		
	}
	while (!keyboard.wentDown(SDLK_SPACE));
}

int main(int argc, char * argv[])
{
	framework.init(GFX_SX, GFX_SY);

	codevember1();

	framework.shutdown();
	
	return 0;
}
