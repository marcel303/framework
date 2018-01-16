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

#include "audioTypes.h"
#include <stddef.h>

struct Wavefield1D
{
	const static int kMaxElems = 2048;
	
	int numElems;
	
	ALIGN32 double p[kMaxElems];
	ALIGN32 double v[kMaxElems];
	ALIGN32 double f[kMaxElems];
	ALIGN32 double d[kMaxElems];
	
	Wavefield1D();
	
	static int roundNumElems(const int numElems);
	
	void init(const int numElems);
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds);
	
	void doGaussianImpact(const int x, const int radius, const double strength);
	float sample(const float x) const;
	
#if AUDIO_USE_SSE
	void * operator new(size_t size);
	void operator delete(void * mem);
#endif
};

//

struct Wavefield1Df
{
	const static int kMaxElems = 2048;
	
	int numElems;
	
	ALIGN32 float p[kMaxElems];
	ALIGN32 float v[kMaxElems];
	ALIGN32 float f[kMaxElems];
	ALIGN32 float d[kMaxElems];
	
	Wavefield1Df();
	
	static int roundNumElems(const int numElems);
	
	void init(const int numElems);
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds);
	
	void doGaussianImpact(const int x, const int radius, const float strength);
	float sample(const float x) const;
	
#if AUDIO_USE_SSE
	void * operator new(size_t size);
	void operator delete(void * mem);
#endif
};

//

struct Wavefield2D
{
	static const int kMaxElems = 64;
	
	int numElems;
	
	ALIGN32 double p[kMaxElems][kMaxElems];
	ALIGN32 double v[kMaxElems][kMaxElems];
	ALIGN32 double f[kMaxElems][kMaxElems];
	ALIGN32 double d[kMaxElems][kMaxElems];
	
	AudioRNG rng;
	
	void init(const int numElems);
	void shut();
	
	Wavefield2D();
	
	static int roundNumElems(const int numElems);
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool _closedEnds);
	void tickForces(const double dt, const double c, const bool _closedEnds);
	void tickVelocity(const double dt, const double vRetainPerSecond, const double pRetainPerSecond);
	
	void randomize();
	
	void doGaussianImpact(const int x, const int y, const int radius, const double strength);
	float sample(const float x, const float y) const;
	
	void copyFrom(const Wavefield2D & other, const bool copyP, const bool copyV, const bool copyF);
	
#if AUDIO_USE_SSE
	void * operator new(size_t size);
	void operator delete(void * mem);
#endif
};

//

struct Wavefield2Df
{
	static const int kMaxElems = 64;
	
	int numElems;
	
	ALIGN32 float p[kMaxElems][kMaxElems];
	ALIGN32 float v[kMaxElems][kMaxElems];
	ALIGN32 float f[kMaxElems][kMaxElems];
	ALIGN32 float d[kMaxElems][kMaxElems];
	
	AudioRNG rng;
	
	void init(const int numElems);
	void shut();
	
	Wavefield2Df();
	
	static int roundNumElems(const int numElems);
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool _closedEnds);
	void tickForces(const float dt, const float c, const bool _closedEnds);
	void tickVelocity(const float dt, const float vRetainPerSecond, const float pRetainPerSecond);
	
	void randomize();
	
	void doGaussianImpact(const int _x, const int _y, const int _radius, const float strength);
	float sample(const float x, const float y) const;
	
	void copyFrom(const Wavefield2Df & other, const bool copyP, const bool copyV, const bool copyF);
	
#if AUDIO_USE_SSE
	void * operator new(size_t size);
	void operator delete(void * mem);
#endif
};
