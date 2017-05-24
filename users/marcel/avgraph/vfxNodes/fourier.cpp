#include "fourier.h"
#include <alloca.h>
#include <cmath>
#include <string.h>

void Fourier::fft1D(
	double * __restrict dreal,
	double * __restrict dimag,
	const int size,
	const int transformSize,
	const bool inverse, const bool normalize)
{
	const int numBits = integerLog2(transformSize);
	
	// pad the data with zeroes
	
	for (int x = size; x < transformSize; ++x)
	{
		const int xReversed = reverseBits(x, numBits);
		
		dreal[xReversed] = 0.0;
		dimag[xReversed] = 0.0;
	}
	
	const double pi2 = M_PI * 2.0;
	const double scale = 1.0 / transformSize;
	
	int n = 1;
	
	for (int k = 0; k < numBits; ++k)
	{
		const int n2 = n;
		
		n <<= 1;
		
		const double angle = (inverse) ? pi2 / n : -pi2 / n;

		double wtmp = std::sin(0.5 * angle);
		double wpr = -2.0 * wtmp * wtmp;
		double wpi = std::sin(angle);
		double wr = 1.0;
		double wi = 0.0;

		for (int m = 0; m < n2; ++m)
		{
			for (int i = m; i < transformSize; i += n)
			{
				const int j = i + n2;
				
				const double tcreal = wr * dreal[j] - wi * dimag[j];
				const double tcimag = wr * dimag[j] + wi * dreal[j];
				
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

void Fourier::fft1D_slow(
	double * __restrict dreal,
	double * __restrict dimag,
	const int size, const int transformSize,
	const bool inverse, const bool normalize)
{
	const int numBits = integerLog2(transformSize);
	
	// create temporary storage needed for index reversal
	
	double * temp = new double[transformSize * 2];
	
	double * __restrict treal = temp + transformSize * 0;
	double * __restrict timag = temp + transformSize * 1;
	
	// reverse the initial data set
	
	for (int x = 0; x < size; ++x)
	{
		const int xReversed = reverseBits(x, numBits);
		
		treal[xReversed] = dreal[x];
		timag[xReversed] = dimag[x];
	}
	
	// perform the fourier pass
	
	fft1D(treal, timag, size, transformSize, inverse, normalize);
	
	// copy the data back. the size of the resulting data is always equal to the transform size
	
	memcpy(dreal, treal, sizeof(double) * transformSize);
	memcpy(dimag, timag, sizeof(double) * transformSize);
	
	// free temporary storage
	
	delete[] temp;
	temp = nullptr;
}

void Fourier::fft2D(
	double * __restrict dreal,
	double * __restrict dimag,
	const int sx, const int transformSx,
	const int sy, const int transformSy,
	const bool inverse, const bool normalize,
	double * __restrict _creal,
	double * __restrict _cimag)
{
	const int numBitsY = integerLog2(transformSy);
	
	// perform FFT on each row
	
	for (int y = 0; y < sy; ++y)
	{
		double * __restrict rreal = &dreal[y * transformSx];
		double * __restrict rimag = &dimag[y * transformSx];
		
		fft1D(rreal, rimag, sx, transformSx, false, false);
	}
	
	// perform FFT on each column
	
	double * __restrict creal;
	double * __restrict cimag;
	
	double * temp = nullptr;
	
	if (_creal && _cimag)
	{
		creal = _creal;
		cimag = _cimag;
	}
	else
	{
		temp = new double[transformSy * 2];
		
		creal = temp + transformSy * 0;
		cimag = temp + transformSy * 1;
	}
	
	int * yReversedLUT = (int*)alloca(sizeof(int) * sy);
	for (int y = 0; y < sy; ++y)
		yReversedLUT[y] = reverseBits(y, numBitsY);
	
	for (int x = 0; x < transformSx; ++x)
	{
		double * __restrict drealc = &dreal[x];
		double * __restrict dimagc = &dimag[x];
		
		for (int y = 0; y < sy; ++y)
		{
			const int yReversed = yReversedLUT[y];
			
			creal[yReversed] = *drealc;
			cimag[yReversed] = *dimagc;
			
			drealc += transformSx;
			dimagc += transformSx;
		}
		
		fft1D(creal, cimag, sy, transformSy, false, false);
		
		//
		
		drealc = &dreal[x];
		dimagc = &dimag[x];
			
		for (int y = 0; y < transformSy; ++y)
		{
			*drealc = creal[y];
			*dimagc = cimag[y];
			
			drealc += transformSx;
			dimagc += transformSx;
		}
	}
	
	delete[] temp;
	temp = nullptr;
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
	const int numBitsX = integerLog2(transformSx);
	
	double * temp = new double[transformSx * transformSy * 2];
	
	double * __restrict treal = temp + transformSx * transformSy * 0;
	double * __restrict timag = temp + transformSx * transformSy * 1;
	
	int * xReversedLUT = (int*)alloca(sizeof(int) * sx);
	for (int x = 0; x < sx; ++x)
		xReversedLUT[x] = reverseBits(x, numBitsX);
	
	for (int y = 0; y < sy; ++y)
	{
		const double * __restrict srcReal = dreal + y * sx;
		const double * __restrict srcImag = dimag + y * sx;
		
		double * __restrict dstReal = treal + y * transformSx;
		double * __restrict dstImag = timag + y * transformSx;
		
		for (int x = 0; x < sx; ++x)
		{
			const int xReversed = xReversedLUT[x];
		
			dstReal[xReversed] = srcReal[x];
			dstImag[xReversed] = srcImag[x];
		}
	}
	
	fft2D(treal, timag, sx, transformSx, sy, transformSy, inverse, normalize, creal, cimag);
	
	memcpy(dreal, treal, sizeof(double) * transformSx * transformSy);
	memcpy(dimag, timag, sizeof(double) * transformSx * transformSy);
	
	delete[] temp;
	temp = nullptr;
}
