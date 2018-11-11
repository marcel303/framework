/*
  WDL - fft.cpp
  Copyright (C) 2006 and later Cockos Incorporated
  Copyright 1999 D. J. Bernstein

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
  


  This file implements the WDL FFT library. These routines are based on the 
  DJBFFT library, which are   Copyright 1999 D. J. Bernstein, djb@pobox.com

  The DJB FFT web page is:  http://cr.yp.to/djbfft.html

*/


// this is based on djbfft

#include <math.h>
#include "fft.h"


#define FFT_MAXBITLEN 15

#ifdef _MSC_VER
#define inline __inline
#endif

#define PI 3.1415926535897932384626433832795

static WDL_FFT_COMPLEX d16[3];
static WDL_FFT_COMPLEX d32[7];
static WDL_FFT_COMPLEX d64[15];
static WDL_FFT_COMPLEX d128[31];
static WDL_FFT_COMPLEX d256[63];
static WDL_FFT_COMPLEX d512[127];
static WDL_FFT_COMPLEX d1024[127];
static WDL_FFT_COMPLEX d2048[255];
static WDL_FFT_COMPLEX d4096[511];
static WDL_FFT_COMPLEX d8192[1023];
static WDL_FFT_COMPLEX d16384[2047];
static WDL_FFT_COMPLEX d32768[4095];


#define sqrthalf (d16[1].re)

// note : adding volatile doesn't work correctly, due to mixed simd (vector) and scalar types used in various places,
//        instead of only scalars as before the simd version was implemented. there may be a possible performance issue
//        with this? or perhaps it was used to speed up compile time due to less register optimisation being necessary
//        for the compiler? in either case it's not worrying me for now
//#define VOL *(volatile WDL_FFT_REAL *)&
#define VOL

#define TRANSFORM(a0,a1,a2,a3,wre,wim) { \
  t6 = a2.re; \
  t1 = a0.re - t6; \
  t6 += a0.re; \
  a0.re = t6; \
  t3 = a3.im; \
  t4 = a1.im - t3; \
  t8 = t1 - t4; \
  t1 += t4; \
  t3 += a1.im; \
  a1.im = t3; \
  t5 = wre; \
  t7 = t8 * t5; \
  t4 = t1 * t5; \
  t8 *= wim; \
  t2 = a3.re; \
  t3 = a1.re - t2; \
  t2 += a1.re; \
  a1.re = t2; \
  t1 *= wim; \
  t6 = a2.im; \
  t2 = a0.im - t6; \
  t6 += a0.im; \
  a0.im = t6; \
  t6 = t2 + t3; \
  t2 -= t3; \
  t3 = t6 * wim; \
  t7 -= t3; \
  a2.re = t7; \
  t6 *= t5; \
  t6 += t8; \
  a2.im = t6; \
  t5 *= t2; \
  t5 -= t1; \
  a3.im = t5; \
  t2 *= wim; \
  t4 += t2; \
  a3.re = t4; \
  }

#define TRANSFORMHALF(a0,a1,a2,a3) { \
  t1 = a2.re; \
  t5 = a0.re - t1; \
  t1 += a0.re; \
  a0.re = t1; \
  t4 = a3.im; \
  t8 = a1.im - t4; \
  t1 = t5 - t8; \
  t5 += t8; \
  t4 += a1.im; \
  a1.im = t4; \
  t3 = a3.re; \
  t7 = a1.re - t3; \
  t3 += a1.re; \
  a1.re = t3; \
  t8 = a2.im; \
  t6 = a0.im - t8; \
  t2 = t6 + t7; \
  t6 -= t7; \
  t8 += a0.im; \
  a0.im = t8; \
  t4 = t6 + t5; \
  t3 = sqrthalf; \
  t4 *= t3; \
  a3.re = t4; \
  t6 -= t5; \
  t6 *= t3; \
  a3.im = t6; \
  t7 = t1 - t2; \
  t7 *= t3; \
  a2.re = t7; \
  t2 += t1; \
  t2 *= t3; \
  a2.im = t2; \
  }

#define TRANSFORMZERO(a0,a1,a2,a3) { \
  t5 = a2.re; \
  t1 = a0.re - t5; \
  t5 += a0.re; \
  a0.re = t5; \
  t8 = a3.im; \
  t4 = a1.im - t8; \
  t7 = a3.re; \
  t6 = t1 - t4; \
  a2.re = t6; \
  t1 += t4; \
  a3.re = t1; \
  t8 += a1.im; \
  a1.im = t8; \
  t3 = a1.re - t7; \
  t7 += a1.re; \
  a1.re = t7; \
  t6 = a2.im; \
  t2 = a0.im - t6; \
  t7 = t2 + t3; \
  a2.im = t7; \
  t2 -= t3; \
  a3.im = t2; \
  t6 += a0.im; \
  a0.im = t6; \
  }

#define UNTRANSFORM(a0,a1,a2,a3,wre,wim) { \
  t6 = VOL wre; \
  t1 = VOL a2.re; \
  t1 *= t6; \
  t8 = VOL wim; \
  t3 = VOL a2.im; \
  t3 *= t8; \
  t2 = VOL a2.im; \
  t4 = VOL a2.re; \
  t5 = VOL a3.re; \
  t5 *= t6; \
  t7 = VOL a3.im; \
  t1 += t3; \
  t7 *= t8; \
  t5 -= t7; \
  t3 = t5 + t1; \
  t5 -= t1; \
  t2 *= t6; \
  t6 *= a3.im; \
  t4 *= t8; \
  t2 -= t4; \
  t8 *= a3.re; \
  t6 += t8; \
  t1 = a0.re - t3; \
  t3 += a0.re; \
  a0.re = t3; \
  t7 = a1.im - t5; \
  t5 += a1.im; \
  a1.im = t5; \
  t4 = t2 - t6; \
  t6 += t2; \
  t8 = a1.re - t4; \
  t4 += a1.re; \
  a1.re = t4; \
  t2 = a0.im - t6; \
  t6 += a0.im; \
  a0.im = t6; \
  a2.re = t1; \
  a3.im = t7; \
  a3.re = t8; \
  a2.im = t2; \
  }
 

#define UNTRANSFORMHALF(a0,a1,a2,a3) { \
  t6 = sqrthalf; \
  t1 = a2.re; \
  t2 = a2.im - t1; \
  t2 *= t6; \
  t1 += a2.im; \
  t1 *= t6; \
  t4 = a3.im; \
  t3 = a3.re - t4; \
  t3 *= t6; \
  t4 += a3.re; \
  t4 *= t6; \
  t8 = t3 - t1; \
  t7 = t2 - t4; \
  t1 += t3; \
  t2 += t4; \
  t4 = a1.im - t8; \
  a3.im = t4; \
  t8 += a1.im; \
  a1.im = t8; \
  t3 = a1.re - t7; \
  a3.re = t3; \
  t7 += a1.re; \
  a1.re = t7; \
  t5 = a0.re - t1; \
  a2.re = t5; \
  t1 += a0.re; \
  a0.re = t1; \
  t6 = a0.im - t2; \
  a2.im = t6; \
  t2 += a0.im; \
  a0.im = t2; \
  }

#define UNTRANSFORMZERO(a0,a1,a2,a3) { \
  t2 = a3.im; \
  t3 = a2.im - t2; \
  t2 += a2.im; \
  t1 = a2.re; \
  t4 = a3.re - t1; \
  t1 += a3.re; \
  t5 = a0.re - t1; \
  a2.re = t5; \
  t6 = a0.im - t2; \
  a2.im = t6; \
  t7 = a1.re - t3; \
  a3.re = t7; \
  t8 = a1.im - t4; \
  a3.im = t8; \
  t1 += a0.re; \
  a0.re = t1; \
  t2 += a0.im; \
  a0.im = t2; \
  t3 += a1.re; \
  a1.re = t3; \
  t4 += a1.im; \
  a1.im = t4; \
  }

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c2(WDL_FFTx_COMPLEX *a)
{
  WDL_FFTx_REAL t1;

  t1 = a[1].re;
  a[1].re = a[0].re - t1;
  a[0].re += t1;

  t1 = a[1].im;
  a[1].im = a[0].im - t1;
  a[0].im += t1;
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static inline void c4(WDL_FFTx_COMPLEX *a)
{
  WDL_FFTx_REAL t1, t2, t3, t4, t5, t6, t7, t8;

  t5 = a[2].re;
  t1 = a[0].re - t5;
  t7 = a[3].re;
  t5 += a[0].re;
  t3 = a[1].re - t7;
  t7 += a[1].re;
  t8 = t5 + t7;
  a[0].re = t8;
  t5 -= t7;
  a[1].re = t5;
  t6 = a[2].im;
  t2 = a[0].im - t6;
  t6 += a[0].im;
  t5 = a[3].im;
  a[2].im = t2 + t3;
  t2 -= t3;
  a[3].im = t2;
  t4 = a[1].im - t5;
  a[3].re = t1 + t4;
  t1 -= t4;
  a[2].re = t1;
  t5 += a[1].im;
  a[0].im = t6 + t5;
  t6 -= t5;
  a[1].im = t6;
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c8(WDL_FFTx_COMPLEX *a)
{
  WDL_FFTx_REAL t1, t2, t3, t4, t5, t6, t7, t8;

  t7 = a[4].im;
  t4 = a[0].im - t7;
  t7 += a[0].im;
  a[0].im = t7;

  t8 = a[6].re;
  t5 = a[2].re - t8;
  t8 += a[2].re;
  a[2].re = t8;

  t7 = a[6].im;
  a[6].im = t4 - t5;
  t4 += t5;
  a[4].im = t4;

  t6 = a[2].im - t7;
  t7 += a[2].im;
  a[2].im = t7;

  t8 = a[4].re;
  t3 = a[0].re - t8;
  t8 += a[0].re;
  a[0].re = t8;

  a[4].re = t3 - t6;
  t3 += t6;
  a[6].re = t3;

  t7 = a[5].re;
  t3 = a[1].re - t7;
  t7 += a[1].re;
  a[1].re = t7;

  t8 = a[7].im;
  t6 = a[3].im - t8;
  t8 += a[3].im;
  a[3].im = t8;
  t1 = t3 - t6;
  t3 += t6;

  t7 = a[5].im;
  t4 = a[1].im - t7;
  t7 += a[1].im;
  a[1].im = t7;

  t8 = a[7].re;
  t5 = a[3].re - t8;
  t8 += a[3].re;
  a[3].re = t8;

  t2 = t4 - t5;
  t4 += t5;

  t6 = t1 - t4;
  t8 = sqrthalf;
  t6 *= t8;
  a[5].re = a[4].re - t6;
  t1 += t4;
  t1 *= t8;
  a[5].im = a[4].im - t1;
  t6 += a[4].re;
  a[4].re = t6;
  t1 += a[4].im;
  a[4].im = t1;

  t5 = t2 - t3;
  t5 *= t8;
  a[7].im = a[6].im - t5;
  t2 += t3;
  t2 *= t8;
  a[7].re = a[6].re - t2;
  t2 += a[6].re;
  a[6].re = t2;
  t5 += a[6].im;
  a[6].im = t5;

  c4<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c16(WDL_FFTx_COMPLEX *a)
{
  WDL_FFTx_REAL t1, t2, t3, t4, t5, t6, t7, t8;

  TRANSFORMZERO(a[0],a[4],a[8],a[12]);
  TRANSFORM(a[1],a[5],a[9],a[13],d16[0].re,d16[0].im);
  TRANSFORMHALF(a[2],a[6],a[10],a[14]);
  TRANSFORM(a[3],a[7],a[11],a[15],d16[0].im,d16[0].re);
  c4<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 8);
  c4<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 12);

  c8<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

/* a[0...8n-1], w[0...2n-2]; n >= 2 */
template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void cpass(WDL_FFTx_COMPLEX *a,const WDL_FFT_COMPLEX *w,unsigned int n)
{
  WDL_FFTx_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  WDL_FFTx_COMPLEX *a1;
  WDL_FFTx_COMPLEX *a2;
  WDL_FFTx_COMPLEX *a3;

  a2 = a + 4 * n;
  a1 = a + 2 * n;
  a3 = a2 + 2 * n;
  --n;

  TRANSFORMZERO(a[0],a1[0],a2[0],a3[0]);
  TRANSFORM(a[1],a1[1],a2[1],a3[1],w[0].re,w[0].im);

  for (;;) {
    TRANSFORM(a[2],a1[2],a2[2],a3[2],w[1].re,w[1].im);
    TRANSFORM(a[3],a1[3],a2[3],a3[3],w[2].re,w[2].im);
    if (!--n) break;
    a += 2;
    a1 += 2;
    a2 += 2;
    a3 += 2;
    w += 2;
  }
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c32(WDL_FFTx_COMPLEX *a)
{
  cpass<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d32,4);
  c8<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 16);
  c8<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 24);
  c16<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c64(WDL_FFTx_COMPLEX *a)
{
  cpass<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d64,8);
  c16<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 32);
  c16<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 48);
  c32<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c128(WDL_FFTx_COMPLEX *a)
{
  cpass<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d128,16);
  c32<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 64);
  c32<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 96);
  c64<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c256(WDL_FFTx_COMPLEX *a)
{
  cpass<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d256,32);
  c64<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 128);
  c64<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 192);
  c128<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c512(WDL_FFTx_COMPLEX *a)
{
  cpass<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d512,64);
  c128<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 384);
  c128<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 256);
  c256<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

/* a[0...8n-1], w[0...n-2]; n even, n >= 4 */
template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void cpassbig(WDL_FFTx_COMPLEX *a,const WDL_FFT_COMPLEX *w,unsigned int n)
{
  WDL_FFTx_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  WDL_FFTx_COMPLEX *a1;
  WDL_FFTx_COMPLEX *a2;
  WDL_FFTx_COMPLEX *a3;
  unsigned int k;

  a2 = a + 4 * n;
  a1 = a + 2 * n;
  a3 = a2 + 2 * n;
  k = n - 2;

  TRANSFORMZERO(a[0],a1[0],a2[0],a3[0]);
  TRANSFORM(a[1],a1[1],a2[1],a3[1],w[0].re,w[0].im);
  a += 2;
  a1 += 2;
  a2 += 2;
  a3 += 2;

  do {
    TRANSFORM(a[0],a1[0],a2[0],a3[0],w[1].re,w[1].im);
    TRANSFORM(a[1],a1[1],a2[1],a3[1],w[2].re,w[2].im);
    a += 2;
    a1 += 2;
    a2 += 2;
    a3 += 2;
    w += 2;
  } while (k -= 2);

  TRANSFORMHALF(a[0],a1[0],a2[0],a3[0]);
  TRANSFORM(a[1],a1[1],a2[1],a3[1],w[0].im,w[0].re);
  a += 2;
  a1 += 2;
  a2 += 2;
  a3 += 2;

  k = n - 2;
  do {
    TRANSFORM(a[0],a1[0],a2[0],a3[0],w[-1].im,w[-1].re);
    TRANSFORM(a[1],a1[1],a2[1],a3[1],w[-2].im,w[-2].re);
    a += 2;
    a1 += 2;
    a2 += 2;
    a3 += 2;
    w -= 2;
  } while (k -= 2);
}


template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c1024(WDL_FFTx_COMPLEX *a)
{
  cpassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d1024,128);
  c256<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 768);
  c256<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 512);
  c512<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c2048(WDL_FFTx_COMPLEX *a)
{
  cpassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d2048,256);
  c512<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 1536);
  c512<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 1024);
  c1024<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c4096(WDL_FFTx_COMPLEX *a)
{
  cpassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d4096,512);
  c1024<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 3072);
  c1024<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 2048);
  c2048<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c8192(WDL_FFTx_COMPLEX *a)
{
  cpassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d8192,1024);
  c2048<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 6144);
  c2048<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 4096);
  c4096<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c16384(WDL_FFTx_COMPLEX *a)
{
  cpassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d16384,2048);
  c4096<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 8192 + 4096);
  c4096<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 8192);
  c8192<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void c32768(WDL_FFTx_COMPLEX *a)
{
  cpassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d32768,4096);
  c8192<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 16384 + 8192);
  c8192<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 16384);
  c16384<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
}


/* n even, n > 0 */
void WDL_fft_complexmul(WDL_FFT_COMPLEX *a,WDL_FFT_COMPLEX *b,int n)
{
  WDL_FFT_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  if (n<2 || (n&1)) return;

  do {
    t1 = a[0].re * b[0].re;
    t2 = a[0].im * b[0].im;
    t3 = a[0].im * b[0].re;
    t4 = a[0].re * b[0].im;
    t5 = a[1].re * b[1].re;
    t6 = a[1].im * b[1].im;
    t7 = a[1].im * b[1].re;
    t8 = a[1].re * b[1].im;
    t1 -= t2;
    t3 += t4;
    t5 -= t6;
    t7 += t8;
    a[0].re = t1;
    a[1].re = t5;
    a[0].im = t3;
    a[1].im = t7;
    a += 2;
    b += 2;
  } while (n -= 2);
}

void WDL_fft_complexmul2(WDL_FFT_COMPLEX *c, WDL_FFT_COMPLEX *a, WDL_FFT_COMPLEX *b, int n)
{
  WDL_FFT_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  if (n<2 || (n&1)) return;

  do {
    t1 = a[0].re * b[0].re;
    t2 = a[0].im * b[0].im;
    t3 = a[0].im * b[0].re;
    t4 = a[0].re * b[0].im;
    t5 = a[1].re * b[1].re;
    t6 = a[1].im * b[1].im;
    t7 = a[1].im * b[1].re;
    t8 = a[1].re * b[1].im;
    t1 -= t2;
    t3 += t4;
    t5 -= t6;
    t7 += t8;
    c[0].re = t1;
    c[1].re = t5;
    c[0].im = t3;
    c[1].im = t7;
    a += 2;
    b += 2;
    c += 2;
  } while (n -= 2);
}
void WDL_fft_complexmul3(WDL_FFT_COMPLEX *c, WDL_FFT_COMPLEX *a, WDL_FFT_COMPLEX *b, int n)
{
  WDL_FFT_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  if (n<2 || (n&1)) return;

  do {
    t1 = a[0].re * b[0].re;
    t2 = a[0].im * b[0].im;
    t3 = a[0].im * b[0].re;
    t4 = a[0].re * b[0].im;
    t5 = a[1].re * b[1].re;
    t6 = a[1].im * b[1].im;
    t7 = a[1].im * b[1].re;
    t8 = a[1].re * b[1].im;
    t1 -= t2;
    t3 += t4;
    t5 -= t6;
    t7 += t8;
    c[0].re += t1;
    c[1].re += t5;
    c[0].im += t3;
    c[1].im += t7;
    a += 2;
    b += 2;
    c += 2;
  } while (n -= 2);
}

/* n even, n > 0 */
void WDL_fft4_complexmul1(WDL_FFT4_COMPLEX *a,WDL_FFT_COMPLEX *b,int n)
{
  WDL_FFT4_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  if (n<2 || (n&1)) return;

  do {
    t1 = a[0].re * b[0].re;
    t2 = a[0].im * b[0].im;
    t3 = a[0].im * b[0].re;
    t4 = a[0].re * b[0].im;
    t5 = a[1].re * b[1].re;
    t6 = a[1].im * b[1].im;
    t7 = a[1].im * b[1].re;
    t8 = a[1].re * b[1].im;
    t1 -= t2;
    t3 += t4;
    t5 -= t6;
    t7 += t8;
    a[0].re = t1;
    a[1].re = t5;
    a[0].im = t3;
    a[1].im = t7;
    a += 2;
    b += 2;
  } while (n -= 2);
}

/* n even, n > 0 */
void WDL_fft4_complexmul4(WDL_FFT4_COMPLEX *a,WDL_FFT4_COMPLEX *b,int n)
{
  WDL_FFT4_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  if (n<2 || (n&1)) return;

  do {
    t1 = a[0].re * b[0].re;
    t2 = a[0].im * b[0].im;
    t3 = a[0].im * b[0].re;
    t4 = a[0].re * b[0].im;
    t5 = a[1].re * b[1].re;
    t6 = a[1].im * b[1].im;
    t7 = a[1].im * b[1].re;
    t8 = a[1].re * b[1].im;
    t1 -= t2;
    t3 += t4;
    t5 -= t6;
    t7 += t8;
    a[0].re = t1;
    a[1].re = t5;
    a[0].im = t3;
    a[1].im = t7;
    a += 2;
    b += 2;
  } while (n -= 2);
}


template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static inline void u4(WDL_FFTx_COMPLEX *a)
{
  WDL_FFTx_REAL t1, t2, t3, t4, t5, t6, t7, t8;

  t1 = VOL a[1].re;
  t3 = a[0].re - t1;
  t6 = VOL a[2].re;
  t1 += a[0].re;
  t8 = a[3].re - t6;
  t6 += a[3].re;
  a[0].re = t1 + t6;
  t1 -= t6;
  a[2].re = t1;

  t2 = VOL a[1].im;
  t4 = a[0].im - t2;
  t2 += a[0].im;
  t5 = VOL a[3].im;
  a[1].im = t4 + t8;
  t4 -= t8;
  a[3].im = t4;

  t7 = a[2].im - t5;
  t5 += a[2].im;
  a[1].re = t3 + t7;
  t3 -= t7;
  a[3].re = t3;
  a[0].im = t2 + t5;
  t2 -= t5;
  a[2].im = t2;
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u8(WDL_FFTx_COMPLEX *a)
{
  WDL_FFTx_REAL t1, t2, t3, t4, t5, t6, t7, t8;

  u4<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);

  t1 = a[5].re;
  a[5].re = a[4].re - t1;
  t1 += a[4].re;

  t3 = a[7].re;
  a[7].re = a[6].re - t3;
  t3 += a[6].re;

  t8 = t3 - t1;
  t1 += t3;

  t6 = a[2].im - t8;
  t8 += a[2].im;
  a[2].im = t8;

  t5 = a[0].re - t1;
  a[4].re = t5;
  t1 += a[0].re;
  a[0].re = t1;

  t2 = a[5].im;
  a[5].im = a[4].im - t2;
  t2 += a[4].im;

  t4 = a[7].im;
  a[7].im = a[6].im - t4;
  t4 += a[6].im;

  a[6].im = t6;

  t7 = t2 - t4;
  t2 += t4;

  t3 = a[2].re - t7;
  a[6].re = t3;
  t7 += a[2].re;
  a[2].re = t7;

  t6 = a[0].im - t2;
  a[4].im = t6;
  t2 += a[0].im;
  a[0].im = t2;

  t6 = sqrthalf;

  t1 = a[5].re;
  t2 = a[5].im - t1;
  t2 *= t6;
  t1 += a[5].im;
  t1 *= t6;
  t4 = a[7].im;
  t3 = a[7].re - t4;
  t3 *= t6;
  t4 += a[7].re;
  t4 *= t6;

  t8 = t3 - t1;
  t1 += t3;
  t7 = t2 - t4;
  t2 += t4;

  t4 = a[3].im - t8;
  a[7].im = t4;
  t5 = a[1].re - t1;
  a[5].re = t5;
  t3 = a[3].re - t7;
  a[7].re = t3;
  t6 = a[1].im - t2;
  a[5].im = t6;

  t8 += a[3].im;
  a[3].im = t8;
  t1 += a[1].re;
  a[1].re = t1;
  t7 += a[3].re;
  a[3].re = t7;
  t2 += a[1].im;
  a[1].im = t2;
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u16(WDL_FFTx_COMPLEX *a)
{
  WDL_FFTx_REAL t1, t2, t3, t4, t5, t6, t7, t8;

  u8<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u4<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 8);
  u4<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 12);

  UNTRANSFORMZERO(a[0],a[4],a[8],a[12]);
  UNTRANSFORMHALF(a[2],a[6],a[10],a[14]);
  UNTRANSFORM(a[1],a[5],a[9],a[13],d16[0].re,d16[0].im);
  UNTRANSFORM(a[3],a[7],a[11],a[15],d16[0].im,d16[0].re);
}

/* a[0...8n-1], w[0...2n-2] */
template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void upass(WDL_FFTx_COMPLEX *a,const WDL_FFT_COMPLEX *w,unsigned int n)
{
  WDL_FFTx_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  WDL_FFTx_COMPLEX *a1;
  WDL_FFTx_COMPLEX *a2;
  WDL_FFTx_COMPLEX *a3;

  a2 = a + 4 * n;
  a1 = a + 2 * n;
  a3 = a2 + 2 * n;
  n -= 1;

  UNTRANSFORMZERO(a[0],a1[0],a2[0],a3[0]);
  UNTRANSFORM(a[1],a1[1],a2[1],a3[1],w[0].re,w[0].im);

  for (;;) {
    UNTRANSFORM(a[2],a1[2],a2[2],a3[2],w[1].re,w[1].im);
    UNTRANSFORM(a[3],a1[3],a2[3],a3[3],w[2].re,w[2].im);
    if (!--n) break;
    a += 2;
    a1 += 2;
    a2 += 2;
    a3 += 2;
    w += 2;
  }
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u32(WDL_FFTx_COMPLEX *a)
{
  u16<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u8<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 16);
  u8<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 24);
  upass<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d32,4);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u64(WDL_FFTx_COMPLEX *a)
{
  u32<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u16<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 32);
  u16<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 48);
  upass<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d64,8);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u128(WDL_FFTx_COMPLEX *a)
{
  u64<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u32<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 64);
  u32<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 96);
  upass<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d128,16);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u256(WDL_FFTx_COMPLEX *a)
{
  u128<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u64<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 128);
  u64<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 192);
  upass<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d256,32);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u512(WDL_FFTx_COMPLEX *a)
{
  u256<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u128<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 256);
  u128<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 384);
  upass<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d512,64);
}


/* a[0...8n-1], w[0...n-2]; n even, n >= 4 */
template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void upassbig(WDL_FFTx_COMPLEX *a,const WDL_FFT_COMPLEX *w,unsigned int n)
{
  WDL_FFTx_REAL t1, t2, t3, t4, t5, t6, t7, t8;
  WDL_FFTx_COMPLEX *a1;
  WDL_FFTx_COMPLEX *a2;
  WDL_FFTx_COMPLEX *a3;
  unsigned int k;

  a2 = a + 4 * n;
  a1 = a + 2 * n;
  a3 = a2 + 2 * n;
  k = n - 2;

  UNTRANSFORMZERO(a[0],a1[0],a2[0],a3[0]);
  UNTRANSFORM(a[1],a1[1],a2[1],a3[1],w[0].re,w[0].im);
  a += 2;
  a1 += 2;
  a2 += 2;
  a3 += 2;

  do {
    UNTRANSFORM(a[0],a1[0],a2[0],a3[0],w[1].re,w[1].im);
    UNTRANSFORM(a[1],a1[1],a2[1],a3[1],w[2].re,w[2].im);
    a += 2;
    a1 += 2;
    a2 += 2;
    a3 += 2;
    w += 2;
  } while (k -= 2);

  UNTRANSFORMHALF(a[0],a1[0],a2[0],a3[0]);
  UNTRANSFORM(a[1],a1[1],a2[1],a3[1],w[0].im,w[0].re);
  a += 2;
  a1 += 2;
  a2 += 2;
  a3 += 2;

  k = n - 2;
  do {
    UNTRANSFORM(a[0],a1[0],a2[0],a3[0],w[-1].im,w[-1].re);
    UNTRANSFORM(a[1],a1[1],a2[1],a3[1],w[-2].im,w[-2].re);
    a += 2;
    a1 += 2;
    a2 += 2;
    a3 += 2;
    w -= 2;
  } while (k -= 2);
}



template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u1024(WDL_FFTx_COMPLEX *a)
{
  u512<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u256<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 512);
  u256<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 768);
  upassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d1024,128);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u2048(WDL_FFTx_COMPLEX *a)
{
  u1024<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u512<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 1024);
  u512<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 1536);
  upassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d2048,256);
}


template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u4096(WDL_FFTx_COMPLEX *a)
{
  u2048<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u1024<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 2048);
  u1024<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 3072);
  upassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d4096,512);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u8192(WDL_FFTx_COMPLEX *a)
{
  u4096<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u2048<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 4096);
  u2048<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 6144);
  upassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d8192,1024);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u16384(WDL_FFTx_COMPLEX *a)
{
  u8192<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u4096<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 8192);
  u4096<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 8192 + 4096);
  upassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d16384,2048);
}

template <typename WDL_FFTx_COMPLEX, typename WDL_FFTx_REAL>
static void u32768(WDL_FFTx_COMPLEX *a)
{
  u16384<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a);
  u8192<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 16384);
  u8192<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a + 16384  + 8192 );
  upassbig<WDL_FFTx_COMPLEX, WDL_FFTx_REAL>(a,d32768,4096);
}


static void __fft_gen(WDL_FFT_COMPLEX *buf, const WDL_FFT_COMPLEX *buf2, int sz, int isfull)
{
  int x;
  double div=PI*0.25/(sz+1);

  if (isfull) div*=2.0;

  for (x = 0; x < sz; x ++)
  {
    if (!(x & 1) || !buf2)
    {
      buf[x].re = (WDL_FFT_REAL) cos((x+1)*div);
      buf[x].im = (WDL_FFT_REAL) sin((x+1)*div);
    }
    else
    {
      buf[x].re = buf2[x >> 1].re;
      buf[x].im = buf2[x >> 1].im;
    }
  }
}

#ifndef WDL_FFT_NO_PERMUTE

static unsigned int fftfreq_c(unsigned int i,unsigned int n)
{
  unsigned int m;

  if (n <= 2) return i;

  m = n >> 1;
  if (i < m) return fftfreq_c(i,m) << 1;

  i -= m;
  m >>= 1;
  if (i < m) return (fftfreq_c(i,m) << 2) + 1;
  i -= m;
  return ((fftfreq_c(i,m) << 2) - 1) & (n - 1);
}

static int _idxperm[2<<FFT_MAXBITLEN];

static void idx_perm_calc(int offs, int n)
{
	int i, j;
	_idxperm[offs] = 0;
	for (i = 1; i < n; ++i) {
		j = fftfreq_c(i, n);
		_idxperm[offs+n-j] = i;
	}
}

int WDL_fft_permute(int fftsize, int idx)
{
  return _idxperm[fftsize+idx-2];
}
int *WDL_fft_permute_tab(int fftsize)
{
  return _idxperm + fftsize - 2;
}


#endif

void WDL_fft_init()
{
  static int ffttabinit;
  if (!ffttabinit)
  {
    int i, offs;
  	ffttabinit=1;

#define fft_gen(x,y,z) __fft_gen(x,y,sizeof(x)/sizeof(x[0]),z)
    fft_gen(d16,0,1);
    fft_gen(d32,d16,1);
    fft_gen(d64,d32,1);
    fft_gen(d128,d64,1);
    fft_gen(d256,d128,1);
    fft_gen(d512,d256,1);
    fft_gen(d1024,d512,0);
    fft_gen(d2048,d1024,0);
    fft_gen(d4096,d2048,0);
    fft_gen(d8192,d4096,0);
    fft_gen(d16384,d8192,0);
    fft_gen(d32768,d16384,0);
#undef fft_gen

#ifndef WDL_FFT_NO_PERMUTE
	  offs = 0;
	  for (i = 2; i <= 32768; i *= 2) 
    {
		  idx_perm_calc(offs, i);
		  offs += i;
	  }
#endif

  }
}

void WDL_fft(WDL_FFT_COMPLEX *buf, int len, int isInverse)
{
  switch (len)
  {
    case 2: c2<WDL_FFT_COMPLEX, WDL_FFT_REAL>(buf); break;
#define TMP(x) case x: if (!isInverse) c##x<WDL_FFT_COMPLEX, WDL_FFT_REAL>(buf); else u##x<WDL_FFT_COMPLEX, WDL_FFT_REAL>(buf); break;
    TMP(4)
    TMP(8)
    TMP(16)
    TMP(32)
    TMP(64)
    TMP(128)
    TMP(256)
    TMP(512)
    TMP(1024)
    TMP(2048)
    TMP(4096)
    TMP(8192)
    TMP(16384)
    TMP(32768)
#undef TMP
  }
}

void WDL_fft4(WDL_FFT4_COMPLEX *buf, int len, int isInverse)
{
  switch (len)
  {
    case 2: c2<WDL_FFT4_COMPLEX, WDL_FFT4_REAL>(buf); break;
#define TMP(x) case x: if (!isInverse) c##x<WDL_FFT4_COMPLEX, WDL_FFT4_REAL>(buf); else u##x<WDL_FFT4_COMPLEX, WDL_FFT4_REAL>(buf); break;
    TMP(4)
    TMP(8)
    TMP(16)
    TMP(32)
    TMP(64)
    TMP(128)
    TMP(256)
    TMP(512)
    TMP(1024)
    TMP(2048)
    TMP(4096)
    TMP(8192)
    TMP(16384)
    TMP(32768)
#undef TMP
  }
}

static inline void r2(WDL_FFT_REAL *a)
{
  WDL_FFT_REAL t1, t2;

  t1 = a[0] + a[1];
  t2 = a[0] - a[1];
  a[0] = t1 * 2;
  a[1] = t2 * 2;
}

static inline void v2(WDL_FFT_REAL *a)
{
  WDL_FFT_REAL t1, t2;

  t1 = a[0] + a[1];
  t2 = a[0] - a[1];
  a[0] = t1;
  a[1] = t2;
}

static void two_for_one(WDL_FFT_REAL* buf, const WDL_FFT_COMPLEX *d, int len, int isInverse)
{
  const unsigned int half = (unsigned)len >> 1, quart = half >> 1, eighth = quart >> 1;
  const int *permute = WDL_fft_permute_tab(half);
  unsigned int i, j;

  WDL_FFT_COMPLEX *p, *q, tw, sum, diff;
  WDL_FFT_REAL tw1, tw2;

  if (!isInverse)
  {
  	WDL_fft((WDL_FFT_COMPLEX*)buf, half, isInverse);
  	r2(buf);
  }
  else
  {
  	v2(buf);
  }

  /* Source: http://www.katjaas.nl/realFFT/realFFT2.html */

  for (i = 1; i < quart; ++i)
  {
    p = (WDL_FFT_COMPLEX*)buf + permute[i];
    q = (WDL_FFT_COMPLEX*)buf + permute[half - i];

/*  tw.re = cos(2*PI * i / len);
    tw.im = sin(2*PI * i / len); */

    if (i < eighth)
    {
      j = i - 1;
      tw.re = d[j].re;
      tw.im = d[j].im;
    }
    else if (i > eighth)
    {
      j = quart - i - 1;
      tw.re = d[j].im;
      tw.im = d[j].re;
    }
    else
    {
      tw.re = tw.im = sqrthalf;
    }

    if (!isInverse) tw.re = -tw.re;

    sum.re = p->re + q->re;
    sum.im = p->im + q->im;
    diff.re = p->re - q->re;
    diff.im = p->im - q->im;

    tw1 = tw.re * sum.im + tw.im * diff.re;
    tw2 = tw.im * sum.im - tw.re * diff.re;

    p->re = sum.re - tw1;
    p->im = diff.im - tw2;
    q->re = sum.re + tw1;
    q->im = -(diff.im + tw2);
  }

  p = (WDL_FFT_COMPLEX*)buf + permute[i];
  p->re *=  2;
  p->im *= -2;

  if (isInverse) WDL_fft((WDL_FFT_COMPLEX*)buf, half, isInverse);
}

void WDL_real_fft(WDL_FFT_REAL* buf, int len, int isInverse)
{
  switch (len)
  {
    case 2: if (!isInverse) r2(buf); else v2(buf); break;
    case 4: case 8: two_for_one(buf, 0, len, isInverse); break;
#define TMP(x) case x: two_for_one(buf, d##x, len, isInverse); break;
    TMP(16)
    TMP(32)
    TMP(64)
    TMP(128)
    TMP(256)
    TMP(512)
    TMP(1024)
    TMP(2048)
    TMP(4096)
    TMP(8192)
    TMP(16384)
    TMP(32768)
#undef TMP
  }
}
