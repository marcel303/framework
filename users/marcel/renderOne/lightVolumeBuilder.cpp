#include "Debugging.h"
#include "lightVolumeBuilder.h"
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

#define OPTIMIZE_LIGHT_SET_CONSTRUCTION 1

#if !OPTIMIZE_LIGHT_SET_CONSTRUCTION
	#include <map>
	#include <set>
#endif

namespace rOne
{
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
		light.type = kLightType_Point;
		light.position = position;
		light.farDistance = radius;

		lights.push_back(light);
	}

	void LightVolumeBuilder::addSpotLight(const int id, Vec3Arg position, Vec3Arg direction, const float angle, const float farDistance)
	{
		AssertMsg(fabsf(direction.CalcSize() - 1.f) <= 1e-3f, "direction vector must be normalized", 0);
		
		Light light;
		light.id = id;
		light.type = kLightType_Spot;
		light.position = position;
		light.direction = direction;
		light.spotAngle = angle;
		light.farDistance = farDistance;

		lights.push_back(light);
	}
	
	void LightVolumeBuilder::addAreaLight(const int id, const Mat4x4 & lightToWorld, const float farDistance)
	{
		Light light;
		light.id = id;
		light.type = kLightType_Area;
		light.areaLightToWorld = lightToWorld;
		light.farDistance = farDistance;
		
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

	LightVolumeData LightVolumeBuilder::generateLightVolumeData(const int halfResolution, const float extents, const bool infiniteSpaceMode) const
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

	#if OPTIMIZE_LIGHT_SET_CONSTRUCTION
		struct Record
		{
			Record * next;
			int lightId;
		};
		
		struct RecordAllocator
		{
			struct Block
			{
				Record records[4096];
			};
			
			Block * currentBlock = nullptr;
			int nextBlockRecordIndex = 0;
			
			std::vector<Block*> blocks;
			
			RecordAllocator()
			{
				blocks.reserve(1024);
			}
			
			~RecordAllocator()
			{
				assert(currentBlock == nullptr);
			}
			
			void cleanup()
			{
				currentBlock = nullptr;
				nextBlockRecordIndex = 0;
				
				for (auto * block : blocks)
					free(block);
				
				blocks.clear();
			}
			
			Record * alloc()
			{
				if (currentBlock == nullptr || nextBlockRecordIndex == 4096)
				{
					Block * block = (Block*)malloc(sizeof(Block));
					blocks.push_back(block);
					
					currentBlock = block;
					nextBlockRecordIndex = 0;
				}
				
				Record * result = currentBlock->records + nextBlockRecordIndex;
				
				nextBlockRecordIndex++;
				
				return result;
			}
		};
		
		RecordAllocator record_allocator;
		
		const int num_records = sx * sy * sz;
		Record ** records = new Record*[num_records];
		memset(records, 0, sizeof(Record*) * num_records);
	#else
		std::map<int, std::set<int>> records;
	#endif

		for (auto & light : lights)
		{
			Vec3 lightMin_world(false);
			Vec3 lightMax_world(false);
			
			if (light.type == kLightType_Point)
			{
				lightMin_world = light.position - Vec3(light.farDistance);
				lightMax_world = light.position + Vec3(light.farDistance);
			}
			else if (light.type == kLightType_Spot)
			{
				computeSpotLightAabb(
					light.position,
					light.direction,
					light.spotAngle,
					light.farDistance,
					lightMin_world,
					lightMax_world);
			}
			else if (light.type == kLightType_Area)
			{
				computeAreaLightAabb(
					light.areaLightToWorld,
					light.farDistance,
					lightMin_world,
					lightMax_world);
			}
			else
			{
				assert(false);
				continue;
			}
			
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
						int indexX = x + extX;
						int indexY = y + extY;
						int indexZ = z + extZ;
						
						if (infiniteSpaceMode)
						{
							indexX = indexX % sx; if (indexX < 0) indexX += sx;
							indexY = indexY % sy; if (indexY < 0) indexY += sy;
							indexZ = indexZ % sz; if (indexZ < 0) indexZ += sz;
						}
						else
						{
							if (indexX < 0 || indexX >= sx ||
								indexY < 0 || indexY >= sy ||
								indexZ < 0 || indexZ >= sz)
							{
								continue;
							}
						}
						
						// optimize : intersect aabb of voxel with the volume of the light. skip non-intersecting voxels

						const int index = indexX + indexY * sx + indexZ * sx * sy;

					#if OPTIMIZE_LIGHT_SET_CONSTRUCTION
						Record * record = record_allocator.alloc();
						record->next = records[index];
						record->lightId = light.id;
						records[index] = record;
					#else
						records[index].insert(light.id);
					#endif
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
		
	#if OPTIMIZE_LIGHT_SET_CONSTRUCTION
		assert(num_cells == num_records);
		
		for (int index = 0; index < num_records; ++index)
		{
			int record_count = 0;
			
			for (Record * record = records[index]; record != nullptr; record = record->next)
				record_count++;
			
			auto & count = index_table[index * 2 + 1];
			
			assert(count == 0);
			
			if (record_count != 0)
			{
				count = record_count;
			}
		}
	#else
		for (auto & record_itr : records)
		{
			const auto index = record_itr.first;
			
			assert(index >= 0 && index < num_cells);
			
			auto & count = index_table[index * 2 + 1];
			
			assert(count == 0);
			
			count = record_itr.second.size();
		}
	#endif
		
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
			#if OPTIMIZE_LIGHT_SET_CONSTRUCTION
				int offset = (int)index_table[i * 2 + 0];
				
				for (Record * record = records[i]; record != nullptr; record = record->next)
				{
					assert(light_ids[offset] == 0);
					
					light_ids[offset] = record->lightId;
					
					offset += 1;
				}
			#else
				auto & src_light_ids = records[i];
				
				assert(src_light_ids.empty() == false);
				
				auto offset = index_table[i * 2 + 0];
				
				for (auto & light_id : src_light_ids)
				{
					assert(light_ids[(int)offset] == 0);
					
					light_ids[(int)offset] = light_id;
					
					offset += 1;
				}
			#endif
			}
		}
		
	#if OPTIMIZE_LIGHT_SET_CONSTRUCTION
		delete [] records;
		records = nullptr;
		
		record_allocator.cleanup();
	#endif

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

	void LightVolumeBuilder::computeSpotLightAabb(
		Vec3Arg position,
		Vec3Arg direction,
		const float angle,
		const float farDistance,
		Vec3 & out_min,
		Vec3 & out_max)
	{
		AssertMsg(fabsf(direction.CalcSize() - 1.f) <= 1e-3f, "direction vector must be normalized", 0);
		
		/*
		steps to compute aabb:
		- compute the four points enclosing the circle at the base of the cone
		- compute the min and max for the four points above, plus the position of the cone
		
		steps to compute the four points enclosing the circle at the base of the cone
		- determine a random vector, that is not parallel to the direction vector
		- use the cross product (twice) to find two vectors extending out of the base of the cone,
		  and at 90 degrees to the direction vector
		- calculate the radius of the circle at the base of the cone, using the angle and far distance
		- calculate the four points using the two vectors above as basis vectors and the radius of the circle
		*/

		// determine the most index of the most prominent axis for the light direction

		int minAxis = 0;
		if (fabsf(direction[1]) < fabsf(direction[minAxis]))
			minAxis = 1;
		if (fabsf(direction[2]) < fabsf(direction[minAxis]))
			minAxis = 2;

		Vec3 anotherVec;
		anotherVec[minAxis] = 1;
		
		const Vec3 tan1 = (anotherVec - direction * (direction * anotherVec)).CalcNormalized();
		const Vec3 tan2 = (direction % tan1).CalcNormalized();

	#if defined(DEBUG)
		const float t1 = direction * tan1;
		const float t2 = direction * tan2;
		assert(fabsf(t1) <= 1e-3f);
		assert(fabsf(t2) <= 1e-3f);
	#endif
	
		const float alpha = tanf(angle/2.f);
		const float radius = alpha * farDistance;

		const Vec3 base = position + direction * farDistance;

		const Vec3 p11 = base - tan1 * radius - tan2 * radius;
		const Vec3 p21 = base + tan1 * radius - tan2 * radius;
		const Vec3 p22 = base + tan1 * radius + tan2 * radius;
		const Vec3 p12 = base - tan1 * radius + tan2 * radius;

	#if defined(DEBUG)
		const float d0 = direction * base;
		const float d11 = direction * p11;
		const float d21 = direction * p21;
		const float d22 = direction * p22;
		const float d12 = direction * p12;

		assert(d11 >= d0 - 1e-3f && d11 <= d0 + 1e-3f);
		assert(d21 >= d0 - 1e-3f && d21 <= d0 + 1e-3f);
		assert(d22 >= d0 - 1e-3f && d22 <= d0 + 1e-3f);
		assert(d12 >= d0 - 1e-3f && d12 <= d0 + 1e-3f);
	#endif

		for (int i = 0; i < 3; ++i)
		{
			out_min[i] = fminf(position[i], fminf(fminf(p11[i], p21[i]), fminf(p22[i], p12[i])));
			out_max[i] = fmaxf(position[i], fmaxf(fmaxf(p11[i], p21[i]), fmaxf(p22[i], p12[i])));
		}
	}
	
	void LightVolumeBuilder::computeAreaLightAabb(
		const Mat4x4 & lightToWorld,
		const float farDistance,
		Vec3 & out_min,
		Vec3 & out_max)
	{
		// by definition, the area light is fully constrained to the
		// box (-1, -1, -1) to (+1, +1, +1) in light-space
		
		// there are eight vertices at the extremities of the box
		// at +1/-1 for all three axis
		
		// we can compute a tight-fitting aabb by transforming all
		// eight vertices and performing min/max operations on the
		// resulting vertices. however, due to symmetries we can
		// get away with testing only four points!
		// I've commented out vertices with a 'negative parity'
		
		const Vec3 vertex_light[4] =
		{
			//{ -1, -1, -1 },
			{ +1, -1, -1 },
			//{ +1, +1, -1 },
			{ -1, +1, -1 },
			
			{ -1, -1, +1 },
			//{ +1, -1, +1 },
			{ +1, +1, +1 },
			//{ -1, +1, +1 },
		};
		
		/*
		static int parity = -1;
		parity = -parity;
		*/
		
		for (int i = 0; i < 4; ++i)
		{
			/*
			float sign = vertex_light[i][0] * vertex_light[i][1] * vertex_light[i][2];
			if (sign != parity)
				continue;
			*/
			
			const Vec3 vertex_world = lightToWorld.Mul4(vertex_light[i]);
			
			if (i == 0)
			{
				out_min = vertex_world;
				out_max = vertex_world;
			}
			else
			{
				out_min = out_min.Min(vertex_world);
				out_max = out_max.Max(vertex_world);
			}
		}
		
		for (int i = 0; i < 3; ++i)
		{
			out_min[i] -= farDistance;
			out_max[i] += farDistance;
		}
	}
}
