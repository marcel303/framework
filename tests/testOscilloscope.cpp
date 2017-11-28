#include "framework.h"

extern const int GFX_SX;
extern const int GFX_SY;

// todo : add to tests menu

void testOscilloscope()
{
	do
	{
		framework.process();
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// code : https://github.com/m1el/woscope
			// demo : http://m1el.github.io/woscope/
			
			const int kNumPoints = 512;
			float x[kNumPoints];
			float y[kNumPoints];
	
			const float m1 = sinf(framework.time / 5.678f) / 10.f;
			const float m2 = sinf(framework.time / 6.789f) / 10.f;
	
			for (int i = 0; i < kNumPoints; ++i)
			{
				x[i] = sinf(i * m1 + framework.time / 1.234f) * 100.f + GFX_SX/2.f;
				y[i] = cosf(i * m2 + framework.time / 2.345f) * 100.f + GFX_SY/2.f;
			}
			
			pushBlend(BLEND_ADD);
			{
				Shader shader("oscline");
				setShader(shader);
				shader.setImmediate("uInvert", 1.f);
				shader.setImmediate("uSize", 4.f);
				shader.setImmediate("uIntensity", .2f);
				shader.setImmediate("uColor", 1.f, 1.f, 1.f, 1.f);
				shader.setImmediate("uNumPoints", kNumPoints);
				
				gxBegin(GL_QUADS);
				{
					for (int i = 0; i < kNumPoints - 1; ++i)
					{
						float x1 = x[i + 0];
						float y1 = y[i + 0];
						float x2 = x[i + 1];
						float y2 = y[i + 1];
						
						setColor(colorWhite);
						for (int j = 0; j < 4; ++j)
							gxVertex4f(x1, y1, x2, y2);
					}
				}
				gxEnd();
				
				clearShader();
			}
			popBlend();
		}
		framework.endDraw();
		
	} while (!keyboard.wentDown(SDLK_SPACE));
}
