#pragma once

#include "framework.h"

struct FluidCube2dGpu
{
	int sizeX;
	int sizeY;

	float dt;
	float diff; // diffusion amount
	float visc; // viscosity

	float voxelSize = 1.f;

	int iter = 4;
	
	Surface s;
	Surface density;
	
// todo : some considerable speedup could probably be had when combining Vx and Vy and Vx0 and Vy0
//        this will require changes to most shaders and functions though
	Surface Vx;
	Surface Vy;
	
	Surface Vx0;
	Surface Vy0;

	void addDensity(const int x, const int y, const float amount);
	void addVelocity(const int x, const int y, const float amountX, const float amountY);

	void step();
};

FluidCube2dGpu * createFluidCube2dGpu(const int sizeX, const int sizeY, const float diffusion, const float viscosity, const float dt);
