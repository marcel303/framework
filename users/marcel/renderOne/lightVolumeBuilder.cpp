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

LightVolumeData LightVolumeBuilder::generateLightVolumeData() const
{
	const int extX = 16;
	const int extZ = 16;

	const int sx = extX * 2 + 1;
	const int sz = extZ * 2 + 1;

	const Vec3 min(-extX, -1000.f, -extZ);
	const Vec3 max(+extX, +1000.f, +extZ);

	std::map<int, std::vector<int>> records;

	for (auto & light : lights)
	{
		const Vec3 lightMin = light.position - Vec3(light.radius, light.radius, light.radius);
		const Vec3 lightMax = light.position + Vec3(light.radius, light.radius, light.radius);

		const int lightMinX = (int)floorf(lightMin[0]);
		const int lightMinZ = (int)floorf(lightMin[2]);

		const int lightMaxX = (int)ceilf(lightMax[0]);
		const int lightMaxZ = (int)ceilf(lightMax[2]);

		for (int x = lightMinX; x <= lightMaxX; ++x)
		{
			for (int z = lightMinZ; z <= lightMaxZ; ++z)
			{
				const int indexX = x + extX;
				const int indexZ = z + extZ;

				if (indexX < 0 || indexX >= sx ||
					indexZ < 0 || indexZ >= sz)
				{
					continue;
				}

				const int index = indexX + indexZ * sx;

				records[index].push_back(light.id);
			}
		}
	}
	
	// convert the records mapping lights to cells to a more GPU friendly data structure
	
	// 1. create a 2d texture encoding start offsets and length for each cell within the frustum
	
	const int num_cells = sx * sz;
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
	
	const int num_light_ids = next_start_offset;
	
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
	result.index_table_sy = sz;
	
	result.light_ids = light_ids;
	result.light_ids_sx = num_light_ids;
	
	return result;
}
