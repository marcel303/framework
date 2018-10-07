/*
  WDL - fft.h
  Copyright (C) 2006 and later Cockos Incorporated
  Copyright (C) 2018 and later Marcel Smit

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
  


Marcel:
  This file is an extension of the WDL FFT library, adding vector-based FFT
  routines which allow one to perform multiple FFTs in one shot. These routines
  are used in the Framework creative coding library and audioGraph to perform
  fast binauralization, performing convolution in the frequency domain using
  multiple filters in one go.

WDL:
  This file defines the interface to the WDL FFT library. These routines are based on the 
  DJBFFT library, which are   Copyright 1999 D. J. Bernstein, djb@pobox.com

  The DJB FFT web page is:  http://cr.yp.to/djbfft.html

*/

#ifndef _WDL_FFT_H_
#define _WDL_FFT_H_

#ifdef __cplusplus
extern "C" {
#endif

#if __SSE2__
	#define WDL_FFT_USE_SSE 1
#else
	#define WDL_FFT_USE_SSE 0
#endif

#if WDL_FFT_USE_SSE
	#include <xmmintrin.h>
#endif

#ifndef WDL_FFT_REALSIZE
#define WDL_FFT_REALSIZE 4
#endif

#if WDL_FFT_REALSIZE == 4
typedef float WDL_FFT_REAL;
#elif WDL_FFT_REALSIZE == 8
typedef double WDL_FFT_REAL;
#else
#error invalid FFT item size
#endif

struct WDL_FFT4_REAL
{
#if WDL_FFT_USE_SSE
	__m128 v;
	
	float elem(const int index) const
	{
		return ((float*)&v)[index];
	}
	
	WDL_FFT4_REAL operator+(const WDL_FFT4_REAL & other) const
	{
		WDL_FFT4_REAL r;
		r.v = _mm_add_ps(v, other.v);
		return r;
	}
	
	WDL_FFT4_REAL operator-(const WDL_FFT4_REAL & other) const
	{
		WDL_FFT4_REAL r;
		r.v = _mm_sub_ps(v, other.v);
		return r;
	}
	
	WDL_FFT4_REAL operator*(const WDL_FFT4_REAL & other) const
	{
		WDL_FFT4_REAL r;
		r.v = _mm_mul_ps(v, other.v);
		return r;
	}
	
	WDL_FFT4_REAL & operator+=(const WDL_FFT4_REAL & other)
	{
		v = _mm_add_ps(v, other.v);
		return *this;
	}
	
	WDL_FFT4_REAL & operator-=(const WDL_FFT4_REAL & other)
	{
		v = _mm_sub_ps(v, other.v);
		return *this;
	}
	
	WDL_FFT4_REAL & operator*=(const WDL_FFT4_REAL & other)
	{
		v = _mm_mul_ps(v, other.v);
		return *this;
	}
	
	//
	
	WDL_FFT4_REAL operator+(const WDL_FFT_REAL other) const
	{
		WDL_FFT4_REAL r;
		r.v = _mm_add_ps(v, _mm_set1_ps(other));
		return r;
	}
	
	WDL_FFT4_REAL operator-(const WDL_FFT_REAL other) const
	{
		WDL_FFT4_REAL r;
		r.v = _mm_sub_ps(v, _mm_set1_ps(other));
		return r;
	}
	
	WDL_FFT4_REAL operator*(const WDL_FFT_REAL other) const
	{
		WDL_FFT4_REAL r;
		r.v = _mm_mul_ps(v, _mm_set1_ps(other));
		return r;
	}
	
	WDL_FFT4_REAL & operator+=(const WDL_FFT_REAL other)
	{
		v = _mm_add_ps(v, _mm_set1_ps(other));
		return *this;
	}
	
	WDL_FFT4_REAL & operator-=(const WDL_FFT_REAL other)
	{
		v = _mm_sub_ps(v, _mm_set1_ps(other));
		return *this;
	}
	
	WDL_FFT4_REAL & operator*=(const WDL_FFT_REAL other)
	{
		v = _mm_mul_ps(v, _mm_set1_ps(other));
		return *this;
	}
	
	WDL_FFT4_REAL & operator=(const WDL_FFT_REAL c)
	{
		v = _mm_set1_ps(c);
		return *this;
	}
#else
	typedef float float4 __attribute__ ((vector_size(16)));
	
	float4 v;
	
	float elem(const int index) const
	{
		return ((float*)&v)[index];
	}
	
	WDL_FFT4_REAL operator+(const WDL_FFT4_REAL & other) const
	{
		WDL_FFT4_REAL r;
		r.v = v + other.v;
		return r;
	}
	
	WDL_FFT4_REAL operator-(const WDL_FFT4_REAL & other) const
	{
		WDL_FFT4_REAL r;
		r.v = v - other.v;
		return r;
	}
	
	WDL_FFT4_REAL operator*(const WDL_FFT4_REAL & other) const
	{
		WDL_FFT4_REAL r;
		r.v = v * other.v;
		return r;
	}
	
	WDL_FFT4_REAL & operator+=(const WDL_FFT4_REAL & other)
	{
		v = v + other.v;
		return *this;
	}
	
	WDL_FFT4_REAL & operator-=(const WDL_FFT4_REAL & other)
	{
		v = v - other.v;
		return *this;
	}
	
	WDL_FFT4_REAL & operator*=(const WDL_FFT4_REAL & other)
	{
		v = v * other.v;
		return *this;
	}
	
	//
	
	WDL_FFT4_REAL operator+(const WDL_FFT_REAL other) const
	{
		WDL_FFT4_REAL r;
		r.v = v + other;
		return r;
	}
	
	WDL_FFT4_REAL operator-(const WDL_FFT_REAL other) const
	{
		WDL_FFT4_REAL r;
		r.v = v - other;
		return r;
	}
	
	WDL_FFT4_REAL operator*(const WDL_FFT_REAL other) const
	{
		WDL_FFT4_REAL r;
		r.v = v * other;
		return r;
	}
	
	WDL_FFT4_REAL & operator+=(const WDL_FFT_REAL other)
	{
		v = v + other;
		return *this;
	}
	
	WDL_FFT4_REAL & operator-=(const WDL_FFT_REAL other)
	{
		v = v - other;
		return *this;
	}
	
	WDL_FFT4_REAL & operator*=(const WDL_FFT_REAL other)
	{
		v = v * other;
		return *this;
	}
	
	WDL_FFT4_REAL & operator=(const WDL_FFT_REAL c)
	{
		v = float4 { c };
		return *this;
	}
#endif
};

typedef struct {
  WDL_FFT_REAL re;
  WDL_FFT_REAL im;
} WDL_FFT_COMPLEX;

struct WDL_FFT4_COMPLEX {
  WDL_FFT4_COMPLEX & operator=(const WDL_FFT_COMPLEX & c)
  {
  #if WDL_FFT_USE_SSE
  	re.v = _mm_set1_ps(c.re);
  	im.v = _mm_set1_ps(c.im);
  #else
  	re.v = WDL_FFT4_REAL::float4 { c.re };
  	im.v = WDL_FFT4_REAL::float4 { c.im };
  #endif
  	return *this;
  }
	
  WDL_FFT4_REAL re;
  WDL_FFT4_REAL im;
};

extern void WDL_fft_init();

extern void WDL_fft_complexmul(WDL_FFT_COMPLEX *dest, WDL_FFT_COMPLEX *src, int len);
extern void WDL_fft_complexmul2(WDL_FFT_COMPLEX *dest, WDL_FFT_COMPLEX *src, WDL_FFT_COMPLEX *src2, int len);
extern void WDL_fft_complexmul3(WDL_FFT_COMPLEX *destAdd, WDL_FFT_COMPLEX *src, WDL_FFT_COMPLEX *src2, int len);

extern void WDL_fft4_complexmul1(WDL_FFT4_COMPLEX *dest, WDL_FFT_COMPLEX *src, int len);
extern void WDL_fft4_complexmul4(WDL_FFT4_COMPLEX *dest, WDL_FFT4_COMPLEX *src, int len);

/* Expects WDL_FFT_COMPLEX input[0..len-1] scaled by 1.0/len, returns
WDL_FFT_COMPLEX output[0..len-1] order by WDL_fft_permute(len). */
extern void WDL_fft(WDL_FFT_COMPLEX *, int len, int isInverse);
extern void WDL_fft4(WDL_FFT4_COMPLEX *, int len, int isInverse);

/* Expects WDL_FFT_REAL input[0..len-1] scaled by 0.5/len, returns
WDL_FFT_COMPLEX output[0..len/2-1], for len >= 4 order by
WDL_fft_permute(len/2). Note that output[len/2].re is stored in
output[0].im. */
extern void WDL_real_fft(WDL_FFT_REAL *, int len, int isInverse);

extern int WDL_fft_permute(int fftsize, int idx);
extern int *WDL_fft_permute_tab(int fftsize);

#ifdef __cplusplus
};
#endif

#endif
