#include "lightVolumeBuilder.h"
#include <assert.h>
#include <math.h>
#include <string.h>

#include <map> // todo : remove. here just for prototyping

//

void LightVolumeData::free()
{
	delete [] index_table;
	index_table = nullptr;
	
	delete [] light_ids;
	light_ids = nullptr;
}

//

void LightVolumeBuilder::addPointLight(const int id, Vec3Arg position, const float radius)
{
	Light light;
	light.id = id;
	light.position = position;
	light.radius = radius;

	lights.push_back(light);
}

void LightVolumeBuilder::reset()
{
	lights.clear();
}

static int roundUp(int value, int multipleOf)
{
	const int n = (value + multipleOf - 1) / multipleOf;
	
	return multipleOf * n;
}

LightVolumeData LightVolumeBuilder::generateLightVolumeData(const int halfResolution, const float extents) const
{
	const int extX = halfResolution;
	const int extY = halfResolution;
	const int extZ = halfResolution;

	const int sx = extX * 2 + 1;
	const int sy = extY * 2 + 1;
	const int sz = extZ * 2 + 1;

	const Vec3 min(-extents, -extents, -extents);
	const Vec3 max(+extents, +extents, +extents);
	
	const float worldToVolumeScale = halfResolution / extents;

	std::map<int, std::vector<int>> records;

	for (auto & light : lights)
	{
		const Vec3 lightMin_world = light.position - Vec3(light.radius, light.radius, light.radius);
		const Vec3 lightMax_world = light.position + Vec3(light.radius, light.radius, light.radius);
		
		const Vec3 lightMin = lightMin_world * worldToVolumeScale;
		const Vec3 lightMax = lightMax_world * worldToVolumeScale;

		const int lightMinX = (int)floorf(lightMin[0]);
		const int lightMinY = (int)floorf(lightMin[1]);
		const int lightMinZ = (int)floorf(lightMin[2]);

		const int lightMaxX = (int)ceilf(lightMax[0]);
		const int lightMaxY = (int)ceilf(lightMax[1]);
		const int lightMaxZ = (int)ceilf(lightMax[2]);

		for (int x = lightMinX; x < lightMaxX; ++x)
		{
			for (int y = lightMinY; y < lightMaxY; ++y)
			{
				for (int z = lightMinZ; z < lightMaxZ; ++z)
				{
					const int indexX = x + extX;
					const int indexY = y + extY;
					const int indexZ = z + extZ;

					if (indexX < 0 || indexX >= sx ||
						indexY < 0 || indexY >= sy ||
						indexZ < 0 || indexZ >= sz)
					{
						continue;
					}

					const int index = indexX + indexY * sx + indexZ * sx * sy;

					records[index].push_back(light.id);
				}
			}
		}
	}
	
	// convert the records mapping lights to cells to a more GPU friendly data structure
	
	// 1. create a 2d texture encoding start offsets and length for each cell within the frustum
	
	const int num_cells = sx * sy * sz;
	float * index_table = new float[num_cells * 2];
	memset(index_table, 0, num_cells * 2 * sizeof(index_table[0]));
	
	// increment counts
	
	for (auto & record_itr : records)
	{
		const auto index = record_itr.first;
		
		assert(index >= 0 && index < num_cells);
		
		auto & count = index_table[index * 2 + 1];
		
		assert(count == 0);
		
		count = record_itr.second.size();
	}
	
	// allocate start offsets
	
	int next_start_offset = 0;
	
	for (int i = 0; i < num_cells; ++i)
	{
		const auto count = index_table[i * 2 + 1];
		
		if (count != 0)
		{
			auto & offset = index_table[i * 2 + 0];
			
			assert(offset == 0);
			
			offset = next_start_offset;
			
			next_start_offset += count;
		}
	}
	
	// 2. create a 1d texture encoding light ids
	
	const int num_light_ids = roundUp(next_start_offset, 4096);
	
	float * light_ids = new float[num_light_ids];
	memset(light_ids, 0, num_light_ids * sizeof(light_ids[0]));
	
	for (int i = 0; i < num_cells; ++i)
	{
		const auto count = index_table[i * 2 + 1];
		
		if (count != 0)
		{
			auto & src_light_ids = records[i];
			
			assert(src_light_ids.empty() == false);
			
			auto offset = index_table[i * 2 + 0];
			
			for (auto & light_id : src_light_ids)
			{
				assert(light_ids[(int)offset] == 0);
				
				light_ids[(int)offset] = light_id;
				
				offset += 1;
			}
		}
	}

	LightVolumeData result;
	
	result.index_table = index_table;
	result.index_table_sx = sx;
	result.index_table_sy = sy;
	result.index_table_sz = sz;
	
	assert((num_light_ids % 4096) == 0);
	result.light_ids = light_ids;
	result.light_ids_sx = 4096;
	result.light_ids_sy = num_light_ids / 4096;
	
	result.world_to_volume_scale = worldToVolumeScale;
	
	return result;
}
