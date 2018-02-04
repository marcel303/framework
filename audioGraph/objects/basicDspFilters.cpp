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

#include "basicDspFilters.h"
#include <complex>
#include <string.h>

const float kBiquadFlatQ = 1.f / sqrtf(2.f);

BiquadFilter::BiquadFilter()
{
	memset(this, 0, sizeof(*this));
}

static std::complex<double> evaluateCoefficients(const double * coeffs, const int numCoefficients, const double w)
{
	std::complex<double> r = coeffs[0];
	
	for (int i = 1; i < numCoefficients; ++i)
	{
		const std::complex<double> c = coeffs[i] * std::exp(std::complex<double>(0.0, -w * i));
		
		r += c;
	}
	
	return r;
}

float evaluateFilter(const double * a, const double * b, const int numCoefficients, const double w)
{
	const std::complex<double> zeroes = evaluateCoefficients(a, numCoefficients, w);
	const std::complex<double> poles  = evaluateCoefficients(b, numCoefficients, w);
	
	const std::complex<double> hw = zeroes / poles;
	
	const double mag = std::abs(hw);
	
	//return 20.0 * std::log10(mag);  // gain in dB
	return mag;
	//phase = np.append(phase, math.atan2(Hw.imag, Hw.real))
}

void evaluateFilter(const double * a, const double * b, const int numCoefficients, const double w1, const double w2, float * magnitude, const int numSteps)
{
	for (int i = 0; i < numSteps; ++i)
	{
		const double t2 = (i + 0.5) / double(numSteps);
		const double t1 = 1.0 - t2;
		const double w = w1 * t1 + w2 * t2;

		magnitude[i] = evaluateFilter(a, b, numCoefficients, w);
	}	
}
