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

extern int GFX_SX;
extern int GFX_SY;

void testImpulseResponseMeasurement()
{
	// todo : generate signal
	
	// todo : do impulse response measurement over time
	
	do
	{
		framework.process();
		
		const int kNumSamples = 1000;
		
		double samples[kNumSamples];
		
		double responseX[kNumSamples];
		double responseY[kNumSamples];
		
		double twoPi = M_PI * 2.0;
		
		double samplePhase = 0.0;
		double sampleStep = twoPi / 87.65;
		
		for (int i = 0; i < kNumSamples; ++i)
		{
			samples[i] = std::cos(samplePhase);
			
			samplePhase = std::fmod(samplePhase + sampleStep, twoPi);
		}
		
		double measurementPhase = twoPi * (mouse.y / double(GFX_SY));
		double measurementStep = twoPi / (150.0 * mouse.x / double(GFX_SX));
		
		if (keyboard.isDown(SDLK_a))
			measurementStep = sampleStep;
		
		for (int i = 0; i < kNumSamples; ++i)
		{
			const double x = std::cos(measurementPhase);
			const double y = std::sin(measurementPhase);
			
			responseX[i] = samples[i] * x;
			responseY[i] = samples[i] * y;
			
			measurementPhase = std::fmod(measurementPhase + measurementStep, twoPi);
		}
		
		double sumS = 0.f;
		double sumX = 0.f;
		double sumY = 0.f;
		
		for (int i = 0; i < kNumSamples; ++i)
		{
			sumS += std::fabs(samples[i]);
			sumX += responseX[i];
			sumY += responseY[i];
		}
		
		const double avgX = sumX / kNumSamples;
		const double avgY = sumY / kNumSamples;
		
		double impulseResponse = std::hypot(avgX, avgY) * 2.0;
		
		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			setColor(colorWhite);
			drawText(GFX_SX/2, GFX_SY/2, 24, 0, 0, "impulse response: %f (%f^8)", impulseResponse, std::pow(impulseResponse, 8.0));
			drawText(GFX_SX/2, GFX_SY/2 + 30, 24, 0, 0, "sampleFreq=%f, measurementFreq=%f", 1.f / sampleStep, 1.f / measurementStep);
			drawText(GFX_SX/2, GFX_SY/2 + 60, 24, 0, 0, "sumS=%f, sumX=%f, sumY=%f", sumS, sumX, sumY);
			
			gxPushMatrix();
			{
				gxTranslatef(0, GFX_SY/2, 0);
				gxScalef(GFX_SX / float(kNumSamples), 1.f, 0.f);
				
				setColor(255, 255, 255);
				gxBegin(GL_POINTS);
				{
					for (int i = 0; i < kNumSamples; ++i)
					{
						gxVertex2f(i, samples[i] * 20.f);
					}
				}
				gxEnd();
				
				setColor(colorRed);
				gxBegin(GL_POINTS);
				{
					float rsum = 0.f;
					
					for (int i = 0; i < kNumSamples; ++i)
					{
						rsum += responseX[i];
						
						gxVertex2f(i, rsum);
						
						gxVertex2f(i, responseX[i] * 20.f);
					}
				}
				gxEnd();
				
				setColor(colorGreen);
				gxBegin(GL_POINTS);
				{
					float rsum = 0.f;
					
					for (int i = 0; i < kNumSamples; ++i)
					{
						rsum += responseY[i];
						
						gxVertex2f(i, rsum);
						
						gxVertex2f(i, responseY[i] * 20.f);
					}
				}
				gxEnd();
			}
			gxPopMatrix();
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
}
