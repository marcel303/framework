#pragma once

#include <assert.h>
#include <stdio.h>
#include "Vec2.h"
#include "Vec3.h"
#include "Vec4.h"

typedef unsigned char UInt8;
typedef unsigned short UInt16;

//#define Log(x, args...) fprintf(stderr, x, args)
#define Log(x) { fprintf(stderr, x); fprintf(stderr, "\n"); }
#define Assert(x) assert(x)

#define SafeFree(x) { delete x; x = 0; }
#define SafeFreeArray(x) { delete[] x; x = 0; }

inline float FastInvSqrt(float v)
{
    float half = 0.5f * v;
	
    int i = *(int*)&v;         // store floating-point bits in integer
    i = 0x5f3759d5 - (i >> 1); // initial guess for Newton's method
	
    v = *(float*)&i;               // convert new bits into float
    v = v * (1.5f - half * v * v);  // One round of Newton's method
	
    return v;
}
