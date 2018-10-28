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

#pragma once

#include <cmath>

#define BIQUAD_OPTIMIZE 1

extern const double kBiquadFlatQ;

enum BiquadType
{
    kBiquadLowpass,
    kBiquadHighpass,
    kBiquadBandpass,
    kBiquadNotch,
    kBiquadPeak,
    kBiquadLowshelf,
    kBiquadHighshelf
};

template <typename real>
struct BiquadFilter
{
	real a0;
	real a1;
	real a2;
	real b1;
	real b2;
	
#if BIQUAD_OPTIMIZE
	real h1;
	real h2;
#else
	real x1;
	real x2;
	real y1;
	real y2;
#endif

	BiquadFilter()
		: a0(0)
		, a1(0)
		, a2(0)
		, b1(0)
		, b2(0)
	#if BIQUAD_OPTIMIZE
		, h1(0)
		, h2(0)
	#else
		, x1(0)
		, x2(0)
		, y1(0)
		, y2(0)
	#endif
	{
	}
	
	void make(const BiquadType type, const real Fc, const real Q, const real peakGainDB)
	{
		switch (type)
		{
		case kBiquadLowpass:
			makeLowpass(Fc, Q, peakGainDB);
			break;
			
		case kBiquadHighpass:
			makeHighpass(Fc, Q, peakGainDB);
			break;
			
		case kBiquadBandpass:
			makeBandpass(Fc, Q, peakGainDB);
			break;
			
		case kBiquadNotch:
			makeNotch(Fc, Q, peakGainDB);
			break;
			
		case kBiquadPeak:
			makePeak(Fc, Q, peakGainDB);
			break;
			
		case kBiquadLowshelf:
			makeLowshelf(Fc, Q, peakGainDB);
			break;
			
		case kBiquadHighshelf:
			makeHighshelf(Fc, Q, peakGainDB);
			break;
		}
	}
	
	void makeLowpass(const real Fc, const real Q, const real peakGainDB)
	{
		const real K = std::tan(real(M_PI) * Fc);
		
		const real norm = 1 / (1 + K / Q + K * K);
		
		a0 = K * K * norm;
		a1 = 2 * a0;
		a2 = a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
	}
	
	void makeHighpass(const real Fc, const real Q, const real peakGainDB)
	{
		const real K = std::tan(real(M_PI) * Fc);
		
		const real norm = 1 / (1 + K / Q + K * K);
		
		a0 = 1 * norm;
		a1 = -2 * a0;
		a2 = a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
	}
	
	void makeBandpass(const real Fc, const real Q, const real peakGainDB)
	{
		const real K = std::tan(real(M_PI) * Fc);
		
		const real norm = 1 / (1 + K / Q + K * K);
		
		a0 = K / Q * norm;
		a1 = 0;
		a2 = -a0;
		b1 = 2 * (K * K - 1) * norm;
		b2 = (1 - K / Q + K * K) * norm;
	}
	
	void makeNotch(const real Fc, const real Q, const real peakGainDB)
	{
		const real K = std::tan(real(M_PI) * Fc);
		
		const real norm = 1 / (1 + K / Q + K * K);
		
		a0 = (1 + K * K) * norm;
		a1 = 2 * (K * K - 1) * norm;
		a2 = a0;
		b1 = a1;
		b2 = (1 - K / Q + K * K) * norm;
	}
	
	void makePeak(const real Fc, const real Q, const real peakGainDB)
	{
		const real K = std::tan(real(M_PI) * Fc);
		const real V = std::pow(real(10), std::abs(peakGainDB) / real(20));
		
		if (peakGainDB >= 0)
		{
			// boost
			const real norm = 1 / (1 + 1/Q * K + K * K);
			a0 = (1 + V/Q * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - V/Q * K + K * K) * norm;
			b1 = a1;
			b2 = (1 - 1/Q * K + K * K) * norm;
		}
		else
		{
			// cut
			const real norm = 1 / (1 + V/Q * K + K * K);
			a0 = (1 + 1/Q * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - 1/Q * K + K * K) * norm;
			b1 = a1;
			b2 = (1 - V/Q * K + K * K) * norm;
		}
	}
	
	void makeLowshelf(const real Fc, const real Q, const real peakGainDB)
	{
		const real K = std::tan(real(M_PI) * Fc);
		const real V = std::pow(real(10), std::abs(peakGainDB) / real(20));
		
		if (peakGainDB >= 0)
		{
			// boost
			const real norm = 1 / (1 + sqrt(2) * K + K * K);
			a0 = (1 + sqrt(2*V) * K + V * K * K) * norm;
			a1 = 2 * (V * K * K - 1) * norm;
			a2 = (1 - sqrt(2*V) * K + V * K * K) * norm;
			b1 = 2 * (K * K - 1) * norm;
			b2 = (1 - sqrt(2) * K + K * K) * norm;
		}
		else
		{
			// cut
			const real norm = 1 / (1 + sqrt(2*V) * K + V * K * K);
			a0 = (1 + sqrt(2) * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - sqrt(2) * K + K * K) * norm;
			b1 = 2 * (V * K * K - 1) * norm;
			b2 = (1 - sqrt(2*V) * K + V * K * K) * norm;
		}
	}
	
	void makeHighshelf(const real Fc, const real Q, const real peakGainDB)
	{
		const real K = std::tan(real(M_PI) * Fc);
		const real V = std::pow(real(10), std::abs(peakGainDB) / real(20));
		
		if (peakGainDB >= 0)
		{
			// boost
			const real norm = 1 / (1 + sqrt(2) * K + K * K);
			a0 = (V + sqrt(2*V) * K + K * K) * norm;
			a1 = 2 * (K * K - V) * norm;
			a2 = (V - sqrt(2*V) * K + K * K) * norm;
			b1 = 2 * (K * K - 1) * norm;
			b2 = (1 - sqrt(2) * K + K * K) * norm;
		}
		else
		{
			// cut
			const real norm = 1 / (V + sqrt(2*V) * K + K * K);
			a0 = (1 + sqrt(2) * K + K * K) * norm;
			a1 = 2 * (K * K - 1) * norm;
			a2 = (1 - sqrt(2) * K + K * K) * norm;
			b1 = 2 * (K * K - V) * norm;
			b2 = (V - sqrt(2*V) * K + K * K) * norm;
		}
	}
	
#if BIQUAD_OPTIMIZE
	real processSingle(const real value)
	{
		const real result = value * a0 + h1;
		
		h1 = value * a1 + h2 - b1 * result;
		h2 = value * a2      - b2 * result;
		
		return result;
	}
#else
	real processSingle(const real x0)
	{
		const real y0 = a0 * x0 + a1 * x1 + a2 * x2 - b1 * y1 - b2 * y2;

		x2 = x1;
		x1 = x0;
		y2 = y1;
		y1 = y0;

		return y0;
	}
#endif
};

//

float evaluateFilter(const double * a, const double * b, const int numCoefficients, const double w);
void evaluateFilter(const double * a, const double * b, const int numCoefficients, const double w1, const double w2, float * magnitude, const int numSteps);
