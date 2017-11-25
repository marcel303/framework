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
#include "testBase.h"

extern const int GFX_SX;
extern const int GFX_SY;

void testHqPrimitives()
{
	setAbout("This example demonstrates how to draw high quality primitives like lines and circles using Framework.");
	
	Surface surface(GFX_SX, GFX_SY, false);
	
	bool doTransform = true;
	bool fixedStrokeSize = false;
	bool enableTexturing = false;
	
	float txTime = 0.f;
	float txSpeed = 0.f;

	do
	{
		framework.process();

		//

		if (keyboard.wentDown(SDLK_a))
			doTransform = !doTransform;
		if (keyboard.wentDown(SDLK_s))
			fixedStrokeSize = !fixedStrokeSize;
		if (keyboard.wentDown(SDLK_t))
			enableTexturing = !enableTexturing;
		
		//
		
		const float dt = framework.timeStep;
		
		if (doTransform)
			txSpeed = std::min(1.f, txSpeed + dt/2.f);
		else
			txSpeed = std::max(0.f, txSpeed - dt/2.f);
		txTime += txSpeed * dt;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			pushSurface(&surface);
			surface.clear(5, 5, 5);
			
			if (enableTexturing)
				gxSetTexture(getTexture("picture.jpg"));
			
			// lines
			
			{
				struct Ve
				{
					Vec2 p;
					Vec2 v;
					float strokeSize;
				};

				const int kMaxVe = 128;
				static int numVe = 0;
				static Ve ve[kMaxVe];
				static bool veInit = true;

				if (veInit || keyboard.isDown(SDLK_i))
				{
					veInit = false;

					numVe = random(0, kMaxVe);

					for (int i = 0; i < numVe; ++i)
					{
						ve[i].p[0] = random(0, GFX_SX);
						ve[i].p[1] = random(0, GFX_SY);
						ve[i].v[0] = random(-30.f, +10.f) * .1f;
						ve[i].v[1] = random(-80.f, +10.f) * .1f;
						ve[i].strokeSize = random(.1f, 1.f);
					}
				}
				else
				{
					for (int i = 0; i < numVe; ++i)
					{
						ve[i].p += ve[i].v * dt;
					}
				}

				//const float strokeSize = 2.f - std::cos(framework.time * .5f) * 1.f;
				const float strokeSize = 0.f + mouse.y / float(GFX_SY) * 10.f;

				{
					gxPushMatrix();
					{
						gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0.f);
						gxRotatef(std::sin(txTime) * 10.f, 0.f, 0.f, 1.f);
						gxScalef(std::cos(txTime / 23.45f) * 1.f, std::cos(txTime / 34.56f) * 1.f, 1.f);
						gxTranslatef(-GFX_SX/2, -GFX_SY/2, 0.f);

						hqBegin(HQ_LINES);
						{
							for (int i = 0; i < numVe/2; ++i)
							{
								const Ve & ve1 = ve[i * 2 + 0];
								const Ve & ve2 = ve[i * 2 + 1];

								const float strokeSize1 = fixedStrokeSize ?  0.f : strokeSize * ve1.strokeSize;
								const float strokeSize2 = fixedStrokeSize ? 12.f : strokeSize * ve2.strokeSize;

								setColor(colorWhite);
								hqLine(ve1.p[0], ve1.p[1], strokeSize1, ve2.p[0], ve2.p[1], strokeSize2);
							}
						}
						hqEnd();
					}
					gxPopMatrix();
				}
			}
			
			//
			
			gxPushMatrix();
			{
				gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);

				hqBegin(HQ_FILLED_TRIANGLES);
				{
					for (int i = 1; i <= 10; ++i)
					{
						setColorf(1.f / i, .5f / i, .25f / i);
						hqFillTriangle(-200.f / i, 0.f, +200.f / i, 0.f, 0.f, +400.f / i);
					}
				}
				hqEnd();

				hqBegin(HQ_STROKED_TRIANGLES);
				{
					for (int i = 1; i <= 10; ++i)
					{
						setColorf(1.f, 1.f, 1.f, 1.f);
						hqStrokeTriangle(-200.f / i, 0.f, +200.f / i, 0.f, 0.f, +400.f / i, 4.f);
					}
				}
				hqEnd();

				hqBegin(HQ_FILLED_CIRCLES);
				{
					for (int i = 1; i <= 10; ++i)
					{
						setColorf(.25f / i, .5f / i, 1.f / i);
						hqFillCircle(0.f, 0.f, 100.f / i);
					}
				}
				hqEnd();

				hqBegin(HQ_STROKED_CIRCLES);
				{
					for (int i = 1; i <= 10; ++i)
					{
						setColor(colorWhite);
						hqStrokeCircle(0.f, 0.f, 100.f / i, 10.f / i + .5f);
					}
				}
				hqEnd();

				hqBegin(HQ_LINES);
				{
					setColor(colorWhite);

					for (int i = 1; i <= 10; ++i)
					{
						hqLine(-100.f, 400.f / i, (i % 2) ? 10-i : 0.f, +100.f, 400.f / i, (i % 2) ? 0.f : 10-i);
					}
				}
				hqEnd();

				gxPushMatrix();
				{
					gxRotatef(-framework.time * 2.f, 0.f, 0.f, 1.f);
					
					hqBegin(HQ_STROKED_RECTS);
					{
						setColor(colorBlack);
						for (int i = 0; i < 10; ++i)
							hqStrokeRect(-50.f - i * 10, -50.f - i * 10, +50.f + i * 10, +50.f + i * 10, i + .5f);
					}
					hqEnd();
				}
				gxPopMatrix();
			}
			gxPopMatrix();
			
			// grid of triangles
			
			gxPushMatrix();
			{
				const float scale = mouse.y / float(GFX_SY) * 10.f;

				gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0.f);
				gxRotatef(std::sin(txTime) * 10.f, 0.f, 0.f, 1.f);
				//gxScalef(std::cos(txTime / 23.45f) * 1.f, std::cos(txTime / 34.56f) * 1.f, 1.f);
				gxScalef(scale, scale, 1.f);

				setColorf(1.f / 10.f, .5f / 10.f, .25f / 10.f);

				hqBegin(HQ_FILLED_TRIANGLES);
				{
					const Vec2 po1(-10.f, 0.f);
					const Vec2 po2(+10.f, 0.f);
					const Vec2 po3(0.f, +20.f);

					for (int t = 0; t < 11 * 11; ++t)
					{
						const int ox = (t % 11) - 5;
						const int oy = (t / 11) - 5;

						if (abs(ox) + abs(oy) <= 3)
							continue;

						const Vec2 o(ox * 20.f, oy * 20.f);

						const Vec2 p1 = po1 + o;
						const Vec2 p2 = po2 + o;
						const Vec2 p3 = po3 + o;

						hqFillTriangle(p1[0], p1[1], p2[0], p2[1], p3[0], p3[1]);
					}
				}
				hqEnd();
			}
			gxPopMatrix();
		
			// stroked concentric circles
			
			gxPushMatrix();
			{
				const float scale = mouse.y / float(GFX_SY) * 10.f;

				gxTranslatef(+GFX_SX/2, +GFX_SY/2, 0.f);
				gxRotatef(std::sin(txTime) * 10.f, 0.f, 0.f, 1.f);
				//gxScalef(std::cos(txTime / 23.45f) * 1.f, std::cos(txTime / 34.56f) * 1.f, 1.f);
				gxScalef(scale, scale, 1.f);

				hqBegin(HQ_FILLED_CIRCLES, true);
				{
					const Vec2 po(0.f, 0.f);
					const float radius = 15.f;

					for (int t = 0; t < 11 * 11; ++t)
					{
						const int ox = (t % 11) - 5;
						const int oy = (t / 11) - 5;

						if (ox == 0 && oy == 0)
							continue;

						const Vec2 o(ox * 20.f, oy * 20.f);
						const Vec2 p = po + o;
							
						hqFillCircle(p[0], p[1], radius);
					}
				}
				hqEnd();
			}
			gxPopMatrix();
			
			gxPushMatrix();
			{
				setColor(127, 127, 127);
				
				gxTranslatef(GFX_SX - 10, GFX_SY - 100, 0);
				
				gxTranslatef(-60, 0, 0);
				hqBegin(HQ_FILLED_ROUNDED_RECTS);
				{
					hqFillRoundedRect(0, 0, 50, 90, 10);
				}
				hqEnd();
				
				gxTranslatef(-60, 0, 0);
				hqBegin(HQ_FILLED_RECTS);
				{
					hqFillRect(0, 0, 50, 90);
				}
				hqEnd();
				
				gxTranslatef(-60, 0, 0);
				drawRect(0, 0, 50, 90);
				hqEnd();
			}
			gxPopMatrix();
			
			gxSetTexture(0);
			
			//
		
			setFont("calibri.ttf");
			//setFontMode(FONT_SDF);
			
			setColor(colorWhite);
			drawText(10.f, 10.f, 24, +1, +1, "move the mouse up and down to animate stroke sizes");
			drawText(10.f, 40.f, 18, +1, +1, "press A to toggle wiggle animation");
			drawText(10.f, 60.f, 18, +1, +1, "press S to toggle fixed size stroke width for lines");
			drawText(10.f, 80.f, 18, +1, +1, "press T to toggle texturing");
			drawText(10.f, 100.f, 18, +1, +1, "press SPACE to quit test");
			
			//
			
			popSurface();
			
			surface.invert();
			
			pushBlend(BLEND_OPAQUE);
			gxSetTexture(surface.getTexture());
			drawRect(0, 0, GFX_SX, GFX_SY);
			gxSetTexture(0);
			popBlend();
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
}
