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

#include "fourier.h"
#include <cmath>
#ifdef WIN32
	#include <malloc.h>
#else
	#include <alloca.h>
#endif
#include <string.h>

//

struct float4impl
{
	__m128 value;
	
	float4impl()
	{
	}
	
	float4impl(const float _value)
	{
		value = _mm_set1_ps(_value);
	}
	
	float4impl(const double _value)
	{
		value = _mm_set1_ps(float(_value));
	}
	
	explicit float4impl(const __m128 _value)
	{
		value = _value;
	}
	
	float4impl & operator=(const float _value)
	{
		value = _mm_set1_ps(_value);
		
		return *this;
	}
	
	float4impl & operator=(const double _value)
	{
		value = _mm_set1_ps(float(_value));
		
		return *this;
	}
	
	float4impl operator-() const
	{
		return float4impl(_mm_sub_ps(_mm_setzero_ps(), value));
	}
	
	float4impl operator+(const float4impl & other) const
	{
		return float4impl(_mm_add_ps(value, other.value));
	}
	
	float4impl operator-(const float4impl & other) const
	{
		return float4impl(_mm_sub_ps(value, other.value));
	}
	
	void operator+=(const float4impl & other)
	{
		value = _mm_add_ps(value, other.value);
	}
	
	float4impl operator*(const float4impl & other) const
	{
		return float4impl(_mm_mul_ps(value, other.value));
	}
	
	void operator*=(const float4impl & other)
	{
		value = _mm_mul_ps(value, other.value);
	}
	
	float4impl operator*(const double other) const
	{
		return float4impl(_mm_mul_ps(value, _mm_set1_ps(float(other))));
	}
	
	float4impl operator/(const int other) const
	{
		return float4impl(_mm_div_ps(value, _mm_set1_ps(float(other))));
	}
};

//

static float sine(const float angle)
{
	return std::sin(angle);
}

static double sine(const double angle)
{
	return std::sin(angle);
}

static float4impl sine(const float4impl & angle)
{
	float4impl result;
	
	float * resultScalar = (float*)&result;
	float * angleScalar = (float*)&angle;
	
	for (int i = 0; i < 4; ++i)
	{
		resultScalar[i] = std::sin(angleScalar[i]);
	}
	
	return result;
}

//

template <typename real>
static void fft1DImpl(
	real * __restrict dreal,
	real * __restrict dimag,
	const int size,
	const int transformSize,
	const bool inverse, const bool normalize)
{
	const int numBits = Fourier::integerLog2(transformSize);
	
	// pad the data with zeroes
	
	for (int x = size; x < transformSize; ++x)
	{
		const int xReversed = Fourier::reverseBits(x, numBits);
		
		dreal[xReversed] = 0.0;
		dimag[xReversed] = 0.0;
	}
	
	const real pi2 = M_PI * 2.0;
	const real scale = 1.0 / transformSize;
	
	int n = 1;
	
	for (int k = 0; k < numBits; ++k)
	{
		const int n2 = n;
		
		n <<= 1;
		
		const real angle = (inverse) ? pi2 / n : -pi2 / n;

		real wtmp = sine(real(0.5) * angle);
		real wpr = real(-2.0) * wtmp * wtmp;
		real wpi = sine(angle);
		real wr = 1.0;
		real wi = 0.0;

		for (int m = 0; m < n2; ++m)
		{
			for (int i = m; i < transformSize; i += n)
			{
				const int j = i + n2;
				
				const real tcreal = wr * dreal[j] - wi * dimag[j];
				const real tcimag = wr * dimag[j] + wi * dreal[j];
				
				dreal[j] = dreal[i] - tcreal;
				dimag[j] = dimag[i] - tcimag;
				
				dreal[i] += tcreal;
				dimag[i] += tcimag;
			}
			
			wtmp = wr;
			wr = wtmp * wpr - wi * wpi + wr;
			wi = wi * wpr + wtmp * wpi + wi;
		}
	}
	
	if (normalize)
	{
		for (int i = 0; i < n; ++i)
		{
			dreal[i] *= scale;
			dimag[i] *= scale;
		}
	}
}

template <typename real>
static void fft1D_slowImpl(
	real * __restrict dreal,
	real * __restrict dimag,
	const int size, const int transformSize,
	const bool inverse, const bool normalize)
{
	const int numBits = Fourier::integerLog2(transformSize);
	
	// create temporary storage needed for index reversal
	
	real * temp = new real[transformSize * 2];
	
	real * __restrict treal = temp + transformSize * 0;
	real * __restrict timag = temp + transformSize * 1;
	
	// reverse the initial data set
	
	for (int x = 0; x < size; ++x)
	{
		const int xReversed = Fourier::reverseBits(x, numBits);
		
		treal[xReversed] = dreal[x];
		timag[xReversed] = dimag[x];
	}
	
	// perform the fourier pass
	
	fft1DImpl(treal, timag, size, transformSize, inverse, normalize);
	
	// copy the data back. the size of the resulting data is always equal to the transform size
	
	memcpy(dreal, treal, sizeof(real) * transformSize);
	memcpy(dimag, timag, sizeof(real) * transformSize);
	
	// free temporary storage
	
	delete[] temp;
	temp = nullptr;
}

void Fourier::fft1D(
	double * __restrict dreal,
	double * __restrict dimag,
	const int size,
	const int transformSize,
	const bool inverse, const bool normalize)
{
	fft1DImpl(dreal, dimag, size, transformSize, inverse, normalize);
}

void Fourier::fft1D_slow(
	double * __restrict dreal,
	double * __restrict dimag,
	const int size, const int transformSize,
	const bool inverse, const bool normalize)
{
	fft1D_slowImpl(dreal, dimag, size, transformSize, inverse, normalize);
}

void Fourier::fft1D(
	float * __restrict dreal,
	float * __restrict dimag,
	const int size,
	const int transformSize,
	const bool inverse, const bool normalize)
{
	fft1DImpl(dreal, dimag, size, transformSize, inverse, normalize);
}

void Fourier::fft1D_slow(
	float * __restrict dreal,
	float * __restrict dimag,
	const int size, const int transformSize,
	const bool inverse, const bool normalize)
{
	fft1D_slowImpl(dreal, dimag, size, transformSize, inverse, normalize);
}

void Fourier::fft1D(
	float4 * __restrict dreal,
	float4 * __restrict dimag,
	const int size,
	const int transformSize,
	const bool inverse, const bool normalize)
{
	float4impl * __restrict dreal_impl = (float4impl*)dreal;
	float4impl * __restrict dimag_impl = (float4impl*)dimag;
	
	fft1DImpl(dreal_impl, dimag_impl, size, transformSize, inverse, normalize);
}

void Fourier::fft1D_slow(
	float4 * __restrict dreal,
	float4 * __restrict dimag,
	const int size, const int transformSize,
	const bool inverse, const bool normalize)
{
	float4impl * __restrict dreal_impl = (float4impl*)dreal;
	float4impl * __restrict dimag_impl = (float4impl*)dimag;
	
	fft1D_slowImpl(dreal_impl, dimag_impl, size, transformSize, inverse, normalize);
}

template <typename real>
static void fft2DImpl(
	real * __restrict dreal,
	real * __restrict dimag,
	const int sx, const int transformSx,
	const int sy, const int transformSy,
	const bool inverse, const bool normalize,
	real * __restrict _creal,
	real * __restrict _cimag)
{
	const int numBitsY = Fourier::integerLog2(transformSy);
	
	// perform FFT on each row
	
	for (int y = 0; y < sy; ++y)
	{
		real * __restrict rreal = &dreal[y * transformSx];
		real * __restrict rimag = &dimag[y * transformSx];
		
		fft1DImpl(rreal, rimag, sx, transformSx, false, false);
	}
	
	// perform FFT on each column
	
	real * __restrict creal;
	real * __restrict cimag;
	
	real * temp = nullptr;
	
	if (_creal && _cimag)
	{
		creal = _creal;
		cimag = _cimag;
	}
	else
	{
		temp = new real[transformSy * 2];
		
		creal = temp + transformSy * 0;
		cimag = temp + transformSy * 1;
	}
	
	int * yReversedLUT = (int*)alloca(sizeof(int) * sy);
	for (int y = 0; y < sy; ++y)
		yReversedLUT[y] = Fourier::reverseBits(y, numBitsY);
	
	for (int x = 0; x < transformSx; ++x)
	{
		{
			real * __restrict drealc = &dreal[x];
			real * __restrict dimagc = &dimag[x];
			
			for (int y = 0; y < sy; ++y)
			{
				const int yReversed = yReversedLUT[y];
				
				creal[yReversed] = *drealc;
				cimag[yReversed] = *dimagc;
				
				drealc += transformSx;
				dimagc += transformSx;
			}
			
			fft1DImpl(creal, cimag, sy, transformSy, false, false);
		}
		
		//
		
		if (normalize)
		{
			real * __restrict drealc = dreal;
			real * __restrict dimagc = dimag;
			
			int offset = x;
		
			const int numValues = transformSx * transformSy;
			const real scale = 1.0 / real(numValues);
	
			for (int y = 0; y < transformSy; ++y)
			{
				drealc[offset] = creal[y] * scale;
				dimagc[offset] = cimag[y] * scale;
				
				offset += transformSx;
			}
		}
		else
		{
			real * __restrict drealc = dreal;
			real * __restrict dimagc = dimag;
			
			int offset = x;
			
			for (int y = 0; y < transformSy; ++y)
			{
				drealc[offset] = creal[y];
				dimagc[offset] = cimag[y];
				
				offset += transformSx;
			}
		}
	}
	
	delete[] temp;
	temp = nullptr;
}

template <typename real>
static void fft2D_slowImpl(
	real * __restrict dreal,
	real * __restrict dimag,
	const int sx, const int transformSx,
	const int sy, const int transformSy,
	const bool inverse, const bool normalize,
	real * __restrict creal,
	real * __restrict cimag)
{
	const int numBitsX = Fourier::integerLog2(transformSx);
	
	real * temp = new real[transformSx * transformSy * 2];
	
	real * __restrict treal = temp + transformSx * transformSy * 0;
	real * __restrict timag = temp + transformSx * transformSy * 1;
	
	int * xReversedLUT = (int*)alloca(sizeof(int) * sx);
	for (int x = 0; x < sx; ++x)
		xReversedLUT[x] = Fourier::reverseBits(x, numBitsX);
	
	for (int y = 0; y < sy; ++y)
	{
		const real * __restrict srcReal = dreal + y * sx;
		const real * __restrict srcImag = dimag + y * sx;
		
		real * __restrict dstReal = treal + y * transformSx;
		real * __restrict dstImag = timag + y * transformSx;
		
		for (int x = 0; x < sx; ++x)
		{
			const int xReversed = xReversedLUT[x];
		
			dstReal[xReversed] = srcReal[x];
			dstImag[xReversed] = srcImag[x];
		}
	}
	
	fft2DImpl(treal, timag, sx, transformSx, sy, transformSy, inverse, normalize, creal, cimag);
	
	memcpy(dreal, treal, sizeof(real) * transformSx * transformSy);
	memcpy(dimag, timag, sizeof(real) * transformSx * transformSy);
	
	delete[] temp;
	temp = nullptr;
}

void Fourier::fft2D(
	double * __restrict dreal,
	double * __restrict dimag,
	const int sx, const int transformSx,
	const int sy, const int transformSy,
	const bool inverse, const bool normalize,
	double * __restrict creal,
	double * __restrict cimag)
{
	fft2DImpl(dreal, dimag, sx, transformSx, sy, transformSy, inverse, normalize, creal, cimag);
}

void Fourier::fft2D_slow(
	double * __restrict dreal,
	double * __restrict dimag,
	const int sx, const int transformSx,
	const int sy, const int transformSy,
	const bool inverse, const bool normalize,
	double * __restrict creal,
	double * __restrict cimag)
{
	fft2D_slowImpl(dreal, dimag, sx, transformSx, sy, transformSy, inverse, normalize, creal, cimag);
}

void Fourier::fft2D(
	float * __restrict dreal,
	float * __restrict dimag,
	const int sx, const int transformSx,
	const int sy, const int transformSy,
	const bool inverse, const bool normalize,
	float * __restrict creal,
	float * __restrict cimag)
{
	fft2DImpl(dreal, dimag, sx, transformSx, sy, transformSy, inverse, normalize, creal, cimag);
}

void Fourier::fft2D_slow(
	float * __restrict dreal,
	float * __restrict dimag,
	const int sx, const int transformSx,
	const int sy, const int transformSy,
	const bool inverse, const bool normalize,
	float * __restrict creal,
	float * __restrict cimag)
{
	fft2D_slowImpl(dreal, dimag, sx, transformSx, sy, transformSy, inverse, normalize, creal, cimag);
}
