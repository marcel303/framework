#pragma once

#include "simd.h"

#if defined(MACOS) || defined(LINUX) || defined(ANDROID)
	#define ALIGN16 __attribute__((aligned(16)))
	#define ALIGN32 __attribute__((aligned(32)))
#else
	#define ALIGN16 __declspec(align(16))
	#define ALIGN32 __declspec(align(32))
#endif

struct Watersim
{
	static const int kMaxElems = 64;
	
	int numElems;
	
	ALIGN32 float p[kMaxElems][kMaxElems];
	ALIGN32 float v[kMaxElems][kMaxElems];
	ALIGN32 float f[kMaxElems][kMaxElems];
	ALIGN32 float d[kMaxElems][kMaxElems];
	
	//AudioRNG rng;
	
	void init(const int numElems);
	void shut();
	
	Watersim();
	
	static int roundNumElems(const int numElems);
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool _closedEnds);
	void tickForces(const float dt, const float c, const bool _closedEnds);
	void tickVelocity(const float dt, const float vRetainPerSecond, const float pRetainPerSecond);
	
	void randomize();
	
	void doGaussianImpact(const int x, const int y, const int radius, const float strength, const float intensity);
	float sample(const float x, const float y) const;
	
	ALIGNED_SIMD_NEW_AND_DELETE();
};
