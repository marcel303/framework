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

static void drawSamples(const int numSamples, const double * samples, const bool doSummation)
{
	if (numSamples < 2)
		return;
	
	if (doSummation)
	{
		double sum = samples[0];
		
		hqBegin(HQ_LINES);
		{
			for (int i = 1; i < numSamples; ++i)
			{
				const double v1 = sum;
				
				sum += samples[i];
				
				const double v2 = sum;
				
				hqLine(
					i + 0, v1, 1.f,
					i + 1, v2, 1.f);
			}
		}
		hqEnd();
	}
	else
	{
		hqBegin(HQ_LINES);
		{
			for (int i = 0; i < numSamples - 1; ++i)
			{
				hqLine(
					i + 0, samples[i + 0] * 20.f, 1.f,
					i + 1, samples[i + 1] * 20.f, 1.f);
			}
		}
		hqEnd();
	}
}

void testImpulseResponseMeasurement()
{
	setAbout("This example demonstrates performance a so called impulse-response measurement. The impulse-response measurement calculates a number between 0 and 1 indicating to what extent two signals share the same frequency.");
	
	double measurementPhaseAnim = 0.0;
	double measurementPhaseAnimSpeed = 0.0;
	
	do
	{
		framework.process();
		
		// stop animating the signal when the mouse moves, and slowly start animating it again once the movement stops
		
		if (mouse.dx || mouse.dy)
			measurementPhaseAnimSpeed = 0.0;
		else
			measurementPhaseAnimSpeed = std::min(1.0, measurementPhaseAnimSpeed + framework.timeStep / 2.0);
		
		// animate the signal by changing the signal's initial phase
		
		measurementPhaseAnim += -1.5 * measurementPhaseAnimSpeed * framework.timeStep;
		
		// construct the signal for which we will do the impulse-response measurement
		
		const int kNumSamples = 1000;
		
		double samples[kNumSamples];
		
		double responseX[kNumSamples];
		double responseY[kNumSamples];
		
		double twoPi = M_PI * 2.0;
		
		double samplePhase = measurementPhaseAnim;
		double sampleStep = twoPi / 87.65;
		
		for (int i = 0; i < kNumSamples; ++i)
		{
			samples[i] = std::cos(samplePhase);
			
			samplePhase = std::fmod(samplePhase + sampleStep + twoPi, twoPi);
		}
		
		// do the impulse-response measurement, by convolving a secondary signal with the signal to be measured
		// if the signals measure in frequency, the impulse-response will be a large number. if they do not
		// match at all, the impulse-response will be close to zero
		
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
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			gxPushMatrix();
			{
				gxTranslatef(0, GFX_SY/2, 0);
				gxScalef(GFX_SX / float(kNumSamples), 1.f, 0.f);
				
				setColor(255, 255, 255);
				drawSamples(kNumSamples, samples, false);
				
				setColor(colorRed);
				drawSamples(kNumSamples, responseX, false);
				drawSamples(kNumSamples, responseX, true);
				
				setColor(colorGreen);
				drawSamples(kNumSamples, responseY, false);
				drawSamples(kNumSamples, responseY, true);
			}
			gxPopMatrix();
			
			//
			
			setFont("calibri.ttf");
			
			setColorf(1, 1, 1, impulseResponse * .8f + .2f);
			const char * text;
			if (impulseResponse < 0.05)
				text = "no match";
			else if (impulseResponse < 0.1)
				text = "getting closer";
			else if (impulseResponse < 0.7)
				text = "close...";
			else
				text = "high impulse-response!";
			drawText(GFX_SX/2, GFX_SX*1/5, 32, 0, 0, "%s", text);
			
			setColor(200, 200, 255);
			drawText(GFX_SX/2, GFX_SY*3/4, 24, 0, 0, "impulse response: %f (%f^8)", impulseResponse, std::pow(impulseResponse, 8.0));
			drawText(GFX_SX/2, GFX_SY*3/4 + 30, 24, 0, 0, "sampleFreq=%f, measurementFreq=%f", 1.f / sampleStep, 1.f / measurementStep);
			drawText(GFX_SX/2, GFX_SY*3/4 + 60, 24, 0, 0, "sumS=%f, sumX=%f, sumY=%f", sumS, sumX, sumY);
			
			setColor(colorWhite);
			drawText(GFX_SX/2, GFX_SY*3/4 + 90, 24, 1, 0, "MOUSE = change frequency");
			drawText(GFX_SX/2, GFX_SY*3/4 + 120, 24, 1, 0, "SPACE = quit");
			
			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
}
