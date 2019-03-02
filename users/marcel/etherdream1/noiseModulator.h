#pragma once

enum NoiseModulatorMode
{
	kNoiseModulatorMode_Simplex,
	kNoiseModulatorMode_Random,
	kNoiseModulatorMode_Sine
};

static const char * NoiseModulatorModeNames[] =
{
	"simplex",
	"random",
	"sine"
};

struct NoiseModulator
{
	float x_current;
	float x_desired;

	float x1 = 0.f;
	float x2 = 0.f;

	float followFactor = .2f;
	
	float noiseFrequency_spat = 20.f;
	float noiseFrequency_time = 20.f;
	
	float strength = 10.f;
	float falloff_strength = 10.f;
	
	float time_phase = 0.f;
	
	bool multiply_strength_with_velocity = false;
	float velocity = 0.f;
	
	NoiseModulatorMode mode = kNoiseModulatorMode_Simplex;
	
	void tick(const float x, const float dt);
	
	float calculateForce(const float in_x, const float in_y) const;
};
