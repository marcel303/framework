#pragma once

#include <vector>

struct FluidCube2d
{
	int size;

	float dt;
	float diff; // diffusion amount
	float visc; // viscosity

	std::vector<float> s;
	std::vector<float> density;

	std::vector<float> Vx;
	std::vector<float> Vy;

	std::vector<float> Vx0;
	std::vector<float> Vy0;

	void addDensity(const int x, const int y, const float amount);
	void addVelocity(const int x, const int y, const float amountX, const float amountY);

	void step();
};

FluidCube2d * createFluidCube2d(const int size, const float diffusion, const float viscosity, const float dt);
