#pragma once

#include "audioTypes.h"

struct Wavefield1D
{
	const static int kMaxElems = 2048;
	
	int numElems;
	
	ALIGN32 double p[kMaxElems];
	ALIGN32 double v[kMaxElems];
	ALIGN32 double f[kMaxElems];
	ALIGN32 double d[kMaxElems];
	
	Wavefield1D();
	
	void init(const int numElems);
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool closedEnds);
	
	float sample(const float x) const;
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
	
	void init(const int numElems);
	void shut();
	
	Wavefield2D();
	
	void tick(const double dt, const double c, const double vRetainPerSecond, const double pRetainPerSecond, const bool _closedEnds);
	void tickForces(const double dt, const double c, const bool _closedEnds);
	void tickVelocity(const double dt, const double vRetainPerSecond, const double pRetainPerSecond);
	
	void randomize();
	
	void doGaussianImpact(const int _x, const int _y, const int _radius, const double strength);
	float sample(const float x, const float y) const;
	
	void copyFrom(const Wavefield2D & other, const bool copyP, const bool copyV, const bool copyF);
};
