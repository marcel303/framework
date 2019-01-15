#include "impulseResponse.h"
#include "lattice.h"
#include <string.h>

void ImpulseResponseState::init(const float * in_frequencies, const int numFrequencies)
{
	memset(this, 0, sizeof(*this));
	
	if (in_frequencies != nullptr)
	{
		for (int i = 0; i < numFrequencies; ++i)
			frequency[i] = in_frequencies[i];
	}
	else
	{
		for (int i = 0; i < kNumProbeFrequencies; ++i)
		{
			//frequency[i] = 40.f * pow(2.f, i / 16.f);
			frequency[i] = 4.f * pow(2.f, i / 16.f);
		}
	}
}

void ImpulseResponseState::restart()
{
	memset(phase, 0, sizeof(phase));
}

void ImpulseResponseState::processBegin(const float in_dt)
{
	dt = in_dt;
	
	for (int i = 0; i < kNumProbeFrequencies; ++i)
	{
		cos_sin[i][0] = cosf(phase[i] * 2.f * M_PI);
		cos_sin[i][1] = sinf(phase[i] * 2.f * M_PI);
	}
}

void ImpulseResponseState::processEnd()
{
	for (int i = 0; i < kNumProbeFrequencies; ++i)
	{
		phase[i] += dt * frequency[i];
		
		phase[i] = fmodf(phase[i], 1.f);
	}
}

//

void ImpulseResponseProbe::init(const int in_vertexIndex)
{
	memset(this, 0, sizeof(*this));
	
	vertexIndex = in_vertexIndex;
}

void ImpulseResponseProbe::measureValue(const ImpulseResponseState & state, const float in_value)
{
	const float value = in_value * state.dt;
	
	for (int i = 0; i < kNumProbeFrequencies; ++i)
	{
		response[i][0] += state.cos_sin[i][0] * value;
		response[i][1] += state.cos_sin[i][1] * value;
	}
}

void ImpulseResponseProbe::measureAtVertex(const ImpulseResponseState & state, const Lattice & lattice)
{
	const auto & p = lattice.vertices_p[vertexIndex];
	const auto & p_init = lattice.vertices_p_init[vertexIndex];
	
	const float dx = p.x - p_init.x;
	const float dy = p.y - p_init.y;
	const float dz = p.z - p_init.z;

	const float value = sqrtf(dx * dx + dy * dy + dz * dz);
	
	measureValue(state, value);
}

void ImpulseResponseProbe::calcResponseMagnitude(float * result) const
{
	for (int i = 0; i < kNumProbeFrequencies; ++i)
		result[i] = hypotf(response[i][0], response[i][1]);
}

float ImpulseResponseProbe::calcResponseMagnitudeForFrequencyIndex(const int frequencyIndex) const
{
	const float result = hypotf(
		response[frequencyIndex][0],
		response[frequencyIndex][1]);
	
	return result;
}
