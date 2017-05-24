#pragma once

/*

note: to use the fft function, the data needs to be inserted into dreal/dimag in bit reversed order first! see the usage example below for how this works. the reason for this is how the FFT algortihm works and to make it possible to perform the FFT in-place
 
note: the input size must be a power of two

example usage:

	const int kN = 64;
	const int kLog2N = Fourier::integerLog2(kN);
	
	double dreal[2][kN];
	double dimag[2][kN];
	
	int a = 0; // we need to reverse bits of data indices before doing the fft. to do so we ping-pong between two sets of data, dreal/dimag[0] and dreal/dimag[1]
	
	// generate some initial data
	
	for (int i = 0; i < kN; ++i)
	{
		dreal[a][i] = std::cos(i / 100.0);
		dimag[a][i] = 0.0;
	}
	
	for (int i = 0; i < kN; ++i)
	{
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
	
	// perform forward pass
	
	for (int i = 0; i < kN; ++i)
	{
		const int sindex = i;
		const int dindex = Fourier::reverseBits(i, kLog2N);
		
		dreal[1 - a][dindex] = dreal[a][sindex];
		dimag[1 - a][dindex] = dimag[a][sindex];
	}
	
	a = 1 - a;
	
	Fourier::fft(dreal[a], dimag[a], kN, kLog2N, false, false);
	
	for (int i = 0; i < kN; ++i)
	{
		logDebug("[%03d] inv=0: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
	
	// perform reverse pass
	
	for (int i = 0; i < kN; ++i)
	{
		const int sindex = i;
		const int dindex = Fourier::reverseBits(i, kLog2N);
		
		dreal[1 - a][dindex] = dreal[a][sindex];
		dimag[1 - a][dindex] = dimag[a][sindex];
	}
	
	a = 1 - a;
	
	Fourier::fft(dreal[a], dimag[a], kN, kLog2N, true, true);
	
	for (int i = 0; i < kN; ++i)
	{
		// note: the values here should be within a small error margin of the original values generated above
 
		logDebug("[%03d] inv=1: dreal=%g, dimag=%g", i, dreal[a][i], dimag[a][i]);
	}
*/

struct Fourier
{
	static void fft(double * __restrict dreal, double * __restrict dimag, const int size, const int log2n, const bool inverse, const bool normalize);

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
