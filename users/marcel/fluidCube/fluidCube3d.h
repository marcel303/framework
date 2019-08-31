#pragma once

#include <vector>

struct FluidCube3d
{
	int size;

	float dt;
	float diff; // diffusion amount
	float visc; // viscosity

	std::vector<float> s;
	std::vector<float> density;

	std::vector<float> Vx;
	std::vector<float> Vy;
	std::vector<float> Vz;

	std::vector<float> Vx0;
	std::vector<float> Vy0;
	std::vector<float> Vz0;
	
	int index(const int x, const int y, const int z) const
	{
		return x + y * size + z * size * size;
	}

	void addDensity(const int x, const int y, const int z, const float amount);
	void addVelocity(const int x, const int y, const int z, const float amountX, const float amountY, const float amountZ);

	void step();
};

FluidCube3d * createFluidCube3d(const int size, const float diffusion, const float viscosity, const float dt);
