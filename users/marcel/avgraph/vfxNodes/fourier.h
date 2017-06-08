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

/*

fft1D:

note: to use the fft1D function, the data needs to be inserted into dreal/dimag in bit reversed order first! see the usage example below for how this works. the reason for this is how the FFT algortihm works and to make it possible to perform the FFT in-place
 
note: the input size must be a power of two

example usage:

	// perform fast fourier transform on 48 (64 output) samples of complex data
	
	const int kSize = 48;
	const int kTransformSize = 64;
	const int kNumBits = Fourier::integerLog2(kTransformSize);
	
	double dreal[2][kTransformSize];
	double dimag[2][kTransformSize];
	
	int a = 0; // we need to reverse bits of data indices before doing the fft. to do so we ping-pong between two sets of data, dreal/dimag[0] and dreal/dimag[1]
	
	for (int i = 0; i < kSize; ++i)
	{
		dreal[a][i] = std::cos(i / 100.0);
		dimag[a][i] = 0.0;
	}
	
	for (int i = 0; i < kSize; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
	
	for (int i = 0; i < kSize; ++i)
	{
		const int sindex = i;
		const int dindex = Fourier::reverseBits(i, kNumBits);
		
		dreal[1 - a][dindex] = dreal[a][sindex];
		dimag[1 - a][dindex] = dimag[a][sindex];
	}
	
	a = 1 - a;
	
	Fourier::fft1D(dreal[a], dimag[a], kSize, kTransformSize, false, false);
	
	for (int i = 0; i < kTransformSize; ++i)
	{
		logDebug("[%03d] inv=0: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
	
	for (int i = 0; i < kTransformSize; ++i)
	{
		const int sindex = i;
		const int dindex = Fourier::reverseBits(i, kNumBits);
		
		dreal[1 - a][dindex] = dreal[a][sindex];
		dimag[1 - a][dindex] = dimag[a][sindex];
	}
	
	a = 1 - a;
	
	Fourier::fft1D(dreal[a], dimag[a], kTransformSize, kTransformSize, true, true);
	
	for (int i = 0; i < kTransformSize; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}

*/

struct Fourier
{
	static void fft1D(
		double * __restrict dreal,
		double * __restrict dimag,
		const int size,
		const int transformSize,
		const bool inverse, const bool normalize);
	static void fft1D_slow(
		double * __restrict dreal,
		double * __restrict dimag,
		const int size, const int transformSize,
		const bool inverse, const bool normalize);
	
	static void fft1D(
		float * __restrict dreal,
		float * __restrict dimag,
		const int size,
		const int transformSize,
		const bool inverse, const bool normalize);
	static void fft1D_slow(
		float * __restrict dreal,
		float * __restrict dimag,
		const int size, const int transformSize,
		const bool inverse, const bool normalize);
	
	static void fft2D(
		double * __restrict dreal,
		double * __restrict dimag,
		const int sx, const int transformSx,
		const int sy, const int transformSy,
		const bool inverse, const bool normalize,
		double * __restrict creal = nullptr,
		double * __restrict cimag = nullptr);
	static void fft2D_slow(
		double * __restrict dreal,
		double * __restrict dimag,
		const int sx, const int transformSx,
		const int sy, const int transformSy,
		const bool inverse, const bool normalize,
		double * __restrict creal = nullptr,
		double * __restrict cimag = nullptr);

	static void fft2D(
		float * __restrict dreal,
		float * __restrict dimag,
		const int sx, const int transformSx,
		const int sy, const int transformSy,
		const bool inverse, const bool normalize,
		float * __restrict creal = nullptr,
		float * __restrict cimag = nullptr);
	static void fft2D_slow(
		float * __restrict dreal,
		float * __restrict dimag,
		const int sx, const int transformSx,
		const int sy, const int transformSy,
		const bool inverse, const bool normalize,
		float * __restrict creal = nullptr,
		float * __restrict cimag = nullptr);
	
	static inline int reverseBits(const int value, const int numBits)
	{
		int smask = 1 << (numBits - 1);
		int dmask = 1;
		
		int result = 0;
		
		for (int i = 0; i < numBits; ++i, smask >>= 1, dmask <<= 1)
		{
			if (value & smask)
				result |= dmask;
		}
		
		return result;
	}

	static inline int upperPowerOf2(int v)
	{
	    v--;
	    v |= v >> 1;
	    v |= v >> 2;
	    v |= v >> 4;
	    v |= v >> 8;
	    v |= v >> 16;
	    v++;
	    return v;
	}

	static inline int integerLog2(int v)
	{
		int r = 0;
		while (v > 1)
		{
			v >>= 1;
			r++;
		}
		return r;
	}
};
