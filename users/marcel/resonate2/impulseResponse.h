#pragma once

#include "constants.h"

struct Lattice;

struct ImpulseResponseState
{
	float frequency[kNumProbeFrequencies];
	float phase[kNumProbeFrequencies];
	
	float cos_sin[kNumProbeFrequencies][2];
	
	float dt = 0.f;
	
	void init(const float * frequencies = nullptr, const int numFrequencies = 0);
	
	void restart();
	
	void processBegin(const float in_dt);
	void processEnd();
};

struct ImpulseResponseProbe
{
	float response[kNumProbeFrequencies][2];
	
	int vertexIndex;
	
	void init(const int in_vertexIndex);
	
	void measureValue(const ImpulseResponseState & state, const float value);
	void measureAtVertex(const ImpulseResponseState & state, const Lattice & lattice);
	
	void calcResponseMagnitude(float * result) const;
	float calcResponseMagnitudeForFrequencyIndex(const int frequencyIndex) const;
};

static inline int calcProbeIndex(const int cubeFaceIndex, const int x, const int y)
{
	const int probeIndex =
		cubeFaceIndex * kProbeGridSize * kProbeGridSize +
		y * kProbeGridSize +
		x;
	
	return probeIndex;
}
