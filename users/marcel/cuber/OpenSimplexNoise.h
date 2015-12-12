#pragma once

class OpenSimplexNoise
{
	uint8_t perm[256];
	uint8_t perm2D[256];
	uint8_t perm3D[256];
	uint8_t perm4D[256];

public:
	static void init();

	OpenSimplexNoise(long seed);

	double Evaluate(double x, double y);
	double Evaluate(double x, double y, double z);
	double Evaluate(double x, double y, double z, double w);
};
