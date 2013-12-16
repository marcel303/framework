#pragma once

#include "../../../libgg/SIMD.h"

class DataVert
{
public:
	SimdVec p;
	//float p[3];
	float uv[2];
};

class DataSurf
{
public:
	DataVert* VertList;
	unsigned int VertCount;
	unsigned int* IndexList;
	unsigned int IndexCount;
};

class Md3
{
public:
	void Load(const char* fileName);

	DataSurf* SurfList;
	unsigned int SurfCount;
};
