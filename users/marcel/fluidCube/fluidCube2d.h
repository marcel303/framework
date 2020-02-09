#pragma once

#include <vector>

struct FluidCube2d
{
	int sizeX;
	int sizeY;

	float dt;
	float diff; // diffusion amount
	float visc; // viscosity
	
	int iter = 4;

	std::vector<float> s;
	std::vector<float> density;

	std::vector<float> Vx;
	std::vector<float> Vy;

	std::vector<float> Vx0;
	std::vector<float> Vy0;

	int index(const int x, const int y) const
	{
	 	return x + y * sizeX;
	}
	
	void addDensity(const int x, const int y, const float amount);
	void addVelocity(const int x, const int y, const float amountX, const float amountY);

	void step();
};

FluidCube2d * createFluidCube2d(const int sizeX, const int sizeY, const float diffusion, const float viscosity, const float dt);
