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

#include "Benchmark.h"
#include "framework.h"
#include "soundmix.h"
#include "testBase.h"

#include "objects/basicDspFilters.h"

extern const int GFX_SX;
extern const int GFX_SY;

/*

DSP filters tested here,

- DC blocking filter. makes sure a signal is centered around zero, even if it has a fixed value offset (called a DC offset in DSP terminology). the offset is subtracted by running an infinite impulse-response measurement over the signal that responds to a very low frequency. the measured impulse response is subtracted from the signal. another way of looking at it is we measure the average signal value over a 'long' time and subtract it from the incoming samples

- one-pole filter. the one-pole filter is a very simple (and very fast) infinite impulse-response filter. it iteratively processes sample using only one filter coefficient. the one-pole filter is suited for building a lowpass filter

- biquad filter. a highly versatile and therefore popular infinite impulse-response filter. quite fast to process and can be used to create low-pass, high-pass, shelf and notch filters

- FIR filter. the finite impulse-response used here convolves an input signal with N coefficients. where convolve just means to pair-wise multiply sample N with coefficient N and sum the results. think of a dot product, but over possibly many more samples. and yes, a dot product is a convolution

filters can be evaluate in various ways. the obvious way is to input 'time'-domain samples to the filter and use the output in further processing, like sending the values to a computer's speakers. another way to evaluate a filter is to run its coefficients through an evaluation function to see what its response is in the frequency domain. this is useful when drawing a graph to preview how a filter will affect an input signal

*/

// a simple helper object to make sure a signal stays centered around zero
// see http://peabody.sapp.org/class/dmp2/lab/dcblock/
// at a = 0.999 the lower audible frequencies are little affected at 44.1 kHz

struct DCBlockingFilter
{
	DCBlockingFilter()
		: previousValue(0.f)
		, previousResult(0.f)
	{
	}

	float process(const float value)
	{
		const double a = 0.999;
		
		const double result = value - previousValue + previousResult * a;
		
		previousValue = value;
		previousResult = result;
		
		return result;
	}

	double previousValue;
	double previousResult;
};

// filter response evaluation as discussed here: http://www.earlevel.com/main/2016/12/01/evaluating-filter-frequency-response/
// v1: straight port of the Python code presented in the above post
// v2: port of the C# version posted in the comments. this version should be faster as it avoids using std::complex and assumes 3 coefficients instead of 'N'

#include <complex>
#include <cmath>

typedef std::complex<double> complexd_t;

static complexd_t evaluateCoefficients(const double * coeffs, const int numCoefficients, const double w)
{
	complexd_t r = coeffs[0];
	
	for (int i = 1; i < numCoefficients; ++i)
	{
		const complexd_t c = coeffs[i] * std::exp(complexd_t(0.0, -w * i));
		
		r += c;
	}
	
	return r;
}

static double evaluateFilter_v1(const double * a, const double * b, const int numCoefficients, const double w)
{
	const complexd_t zeroes = evaluateCoefficients(a, numCoefficients, w);
	const complexd_t poles  = evaluateCoefficients(b, numCoefficients, w);
	
	const complexd_t hw = zeroes / poles;
	
	const double mag = std::abs(hw);
	
	//return 20.0 * std::log10(mag);  // gain in dB
	return mag;
	//phase = np.append(phase, math.atan2(Hw.imag, Hw.real))
}

static double evaluateFilter_v2(
	const double freq,
	const double sampleRate,
	const double a0, const double a1, const double a2,
                     const double b1, const double b2)
{
	const double w = 2.0 * M_PI * freq / sampleRate;

	const double cos1 = std::cos(-1 * w);
	const double sin1 = std::sin(-1 * w);
	
	const double cos2 = std::cos(-2 * w);
	const double sin2 = std::sin(-2 * w);
	
	const double realZeros = a0 + a1 * cos1 + a2 * cos2;
	const double imagZeros =      a1 * sin1 + a2 * sin2;

	const double realPoles = 1.0 + b1 * cos1 + b2 * cos2;
	const double imagPoles =       b1 * sin1 + b2 * sin2;

	const double divider = realPoles * realPoles + imagPoles * imagPoles;

	const double realHw = (realZeros * realPoles + imagZeros * imagPoles) / divider;
	const double imagHw = (imagZeros * realPoles - realZeros * imagPoles) / divider;

	const double magnitude = std::sqrt(realHw * realHw + imagHw * imagHw);
	//const double phase = std::atan2(imagHw, realHw);

	//return magnitude;     //gain in Au
	return 20.0 * std::log10(magnitude);  // gain in dB
	//return phase;                   //phase
}

// finite impulse response filter. convolves the input with a set of N coefficients to produce the result
template <int SIZE>
struct FirFilter
{
	static const int HISTORY_SIZE = SIZE - 1;
	
	float workBuffer[HISTORY_SIZE + AUDIO_UPDATE_SIZE];
	
	FirFilter()
	{
		memset(workBuffer, 0, HISTORY_SIZE * sizeof(float));
	}
	
	void process(
		const float * __restrict coefficients,
		const float * __restrict inputSamples,
		const int numInputSamples,
		float * __restrict outputSamples)
	{
		Assert(numInputSamples <= AUDIO_UPDATE_SIZE);
		
		// append the input samples to the history
		
		memcpy(workBuffer + HISTORY_SIZE, inputSamples, numInputSamples * sizeof(float));
		
		// convolve numInputSamples from the work buffer with the coefficients passed in
		
		for (int i = 0; i < numInputSamples; ++i)
		{
			float sum = 0.f;
			
			const float * __restrict workBufferPtr = workBuffer + HISTORY_SIZE + i;
			
			for (int x = 0; x < SIZE; ++x)
			{
				sum += coefficients[x] * workBufferPtr[-x];
			}
			
			outputSamples[i] = sum;
		}
		
		// copy the last few samples to the start of the work buffer. they will serve as the new history values
		
		memmove(workBuffer, workBuffer + numInputSamples, HISTORY_SIZE * sizeof(float));
	}
};

// see http://www.earlevel.com/main/2012/12/15/a-one-pole-filter/ for a discussion about one pole filters
// a one pole filter is suitable for implementing a lowpass filter, but not so much as highpass filter (see the article for an explanation why)
struct OnePoleFilter
{
	float a0;
	float b1;
	
	float h1;
	
	OnePoleFilter()
	{
		memset(this, 0, sizeof(*this));
	}
	
	void makeLowpass(const float f)
	{
		b1 = -std::exp(-2.0 * M_PI * f);
		a0 = 1.f - fabsf(b1);
	}
	
	void makeHighpass(const float f)
	{
		b1 = +std::exp(-2.0 * M_PI * (0.5 - f));
		a0 = 1.f - fabsf(b1);
	}
	
	float processSingle(const float value)
	{
		const float result = a0 * value - b1 * h1;
		
		h1 = result;
		
		return result;
	}
};

void testDspFilters()
{
	setAbout("This test uses various techniques from the field of digital signal processing (DSP), to evaluate and run filters across a signal.");
	
#if 1
	DCBlockingFilter f;
	
	for (int i = 0; i < 1000; ++i)
	{
		const double v = 1.f;
		
		const double s = f.process(v);
		
		printf("DCBlockingFilter: %.2f -> %.2f\n", v, s);
	}
#endif

#if 1
	{
		FirFilter<32> firFilter;
		
		float coefficients[32];
		for (int i = 0; i < 32; ++i)
			coefficients[i] = random(-i / 32.f, +1.f);
		
		float srcSamples[AUDIO_UPDATE_SIZE];
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			srcSamples[i] = sinf(i / float(AUDIO_UPDATE_SIZE) * M_PI * 4);
		
		float dstSamples[AUDIO_UPDATE_SIZE];
		for (int i = 0; i < AUDIO_UPDATE_SIZE; i += 32)
			firFilter.process(coefficients, srcSamples + i, 32, dstSamples + i);
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			printf("FirFilter: %.2f -> %.2f\n", srcSamples[i], dstSamples[i]);
	}
#endif

#if 1
	{
		OnePoleFilter onePoleFilter;
		onePoleFilter.makeLowpass(.1f);
		
		float srcSamples[AUDIO_UPDATE_SIZE];
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			srcSamples[i] = sinf(i / float(AUDIO_UPDATE_SIZE) * M_PI * 20);
		
		float dstSamples[AUDIO_UPDATE_SIZE];
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			dstSamples[i] = onePoleFilter.processSingle(srcSamples[i]);
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			printf("OnePoleFilter: %.2f -> %.2f\n", srcSamples[i], dstSamples[i]);
	}
#endif

#if 1
	{
		BiquadFilter biquadFilter;
		biquadFilter.makeLowpass(.1f, .1f, 1.f);
		
		float srcSamples[AUDIO_UPDATE_SIZE];
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			srcSamples[i] = sinf(i / float(AUDIO_UPDATE_SIZE) * M_PI * 20);
		
		float dstSamples[AUDIO_UPDATE_SIZE];
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			dstSamples[i] = biquadFilter.processSingle(srcSamples[i]);
		
		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
			printf("BiquadFilter: %.2f -> %.2f\n", srcSamples[i], dstSamples[i]);
	}
#endif

	do
	{
		framework.process();
		
		double a[3] = { 0.25137900151315906, 0.5027580030263181, 0.25137900151315906 };
		double b[3] = { 1.0, -0.17124071441396305, 0.17675672046659927 };
		int numCoefficients = 3;
		
		a[0] = lerp(0.0, 0.1, (sin(framework.time / 1.23) + 1.0) / 2.0);
		a[1] = lerp(0.0, 0.1, (sin(framework.time / 2.34) + 1.0) / 2.0);
		a[2] = lerp(0.0, 0.1, (sin(framework.time / 3.45) + 1.0) / 2.0);
		
		b[1] = lerp(-0.1, +0.1, (sin(framework.time / 1.56) + 1.0) / 2.0);
		b[2] = lerp(-0.1, +0.1, (sin(framework.time / 1.67) + 1.0) / 2.0);
		
		const float f = lerp(0.f, .5f, (cosf(framework.time / 3.45) + 1.f) / 2.f);
		
		if (keyboard.isDown(SDLK_b) || 1)
		{
			BiquadFilter biquadFilter;
			//biquadFilter.makeLowpass(f, mouse.x / double(GFX_SX), 1.0);
			//biquadFilter.makeHighpass(mouse.x / double(GFX_SX) / 2.0, f, 1.0);
			//biquadFilter.makeNotch(mouse.x / double(GFX_SX) / 2.0, f, 1.0);
			//biquadFilter.makeLowshelf(mouse.x / double(GFX_SX) / 2.0, f, 6.0);
			//biquadFilter.makeHighshelf(mouse.x / double(GFX_SX) / 2.0, f, 6.0);
			biquadFilter.makePeak(mouse.x / double(GFX_SX) / 2.0, f, 6.0);
		
			a[0] = biquadFilter.a0;
			a[1] = biquadFilter.a1;
			a[2] = biquadFilter.a2;
			
			b[0] = 1.f;
			b[1] = biquadFilter.b1;
			b[2] = biquadFilter.b2;
			
			numCoefficients = 3;
		}
		else if (keyboard.isDown(SDLK_o))
		{
			OnePoleFilter onePoleFilter;
			if (keyboard.isDown(SDLK_LSHIFT) || keyboard.isDown(SDLK_RSHIFT))
				onePoleFilter.makeHighpass(f);
			else
				onePoleFilter.makeLowpass(f);
		
			a[0] = onePoleFilter.a0;
			a[1] = 0.f;
			a[2] = 0.f;
			
			b[0] = 1.f;
			b[1] = onePoleFilter.b1;
			b[2] = 0.f;
			
			numCoefficients = 2;
		}
		
		const int n = 1024;
		double response[n];
		
		{
			Benchmark bm("evaluate filter");
			
			for (int i = 0; i < n; ++i)
			{
				if (keyboard.isDown(SDLK_SPACE))
				{
					response[i] = evaluateFilter_v2(i / 2.0, n, a[0], a[1], a[2], b[1], b[2]);
				}
				else
				{
					const double w = i * M_PI / (n - 1);
					
					response[i] = evaluateFilter_v1(a, b, numCoefficients, w);
				}
			}
		}

		framework.beginDraw(0, 0, 0, 0);
		{
			setFont("calibri.ttf");
			
			hqBegin(HQ_LINES);
			{
				const float scale = -100.f;
				
				setColor(colorGreen);
				hqLine(0, GFX_SY/2, 1.f, GFX_SX, GFX_SY/2, 1.f);
				
				setColor(colorRed);
				hqLine(0, GFX_SY/2 + scale, 1.f, GFX_SX, GFX_SY/2 + scale, 1.f);
				
				for (int i = 0; i < n - 1; ++i)
				{
					const float x1 = GFX_SX/2 - n/2 + i + 0;
					const float x2 = GFX_SX/2 - n/2 + i + 1;
					const float y1 = response[i + 0] * scale;
					const float y2 = response[i + 1] * scale;
					
					setColor(colorWhite);
					hqLine(x1, GFX_SY/2 + y1, 3.f, x2, GFX_SY/2 + y2, 3.f);
				}
			}
			hqEnd();

			setColor(colorGreen);
			hqBegin(HQ_FILLED_CIRCLES);
			{
				hqFillCircle(GFX_SX/2 + (f - .25f) * n * 2, 50, 10);
			}
			hqEnd();

			drawTestUi();
		}
		framework.endDraw();
	} while (tickTestUi());
}
