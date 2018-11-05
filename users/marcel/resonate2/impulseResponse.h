#pragma once

#include "constants.h"

struct Lattice;

struct ImpulseResponsePhaseState
{
	float frequency[kNumProbeFrequencies];
	float phase[kNumProbeFrequencies];
	
	float cos_sin[kNumProbeFrequencies][2];
	
	float dt = 0.f;
	
	void init(const float * frequencies = nullptr, const int numFrequencies = 0);
	
	void processBegin(const float in_dt);
	void processEnd();
};

struct ImpulseResponseProbe
{
	float response[kNumProbeFrequencies][2];
	
	int vertexIndex;
	
	void init(const int in_vertexIndex);
	
	void measureValue(const ImpulseResponsePhaseState & state, const float value);
	void measureAtVertex(const ImpulseResponsePhaseState & state, const Lattice & lattice);
	
	void calcResponseMagnitude(float * result) const;
};
