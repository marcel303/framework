#pragma once

#include "Vec3.h"
#include <vector>

struct LightVolumeData
{
	float * index_table = nullptr;
	int index_table_sx = 0;
	int index_table_sy = 0;
	
	float * light_ids = nullptr;
	int light_ids_sx = 0;
	
	void free();
};

class LightVolumeBuilder
{
	struct Light
	{
		int id;

		Vec3 position;
		float radius;
	};

	std::vector<Light> lights;

public:

	void addPointLight(const int id, Vec3Arg position, const float radius);
	void reset();

	LightVolumeData generateLightVolumeData() const;
};
