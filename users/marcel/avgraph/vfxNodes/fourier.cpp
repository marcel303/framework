#include "fourier.h"
#include <cmath>

void Fourier::fft(double * __restrict dreal, double * __restrict dimag, const int size, const int log2n, const bool inverse, const bool normalize)
{
	const double pi2 = M_PI * 2.0;
	const double scale = 1.0 / size;
	
	int n = 1;
	
	for (int k = 0; k < log2n; ++k)
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
			for (int i = m; i < size; i += n)
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
