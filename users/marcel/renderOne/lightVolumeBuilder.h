#pragma once

#include "Vec3.h"
#include <vector>

namespace rOne
{
	struct LightVolumeData
	{
		float * index_table = nullptr;
		int index_table_sx = 0;
		int index_table_sy = 0;
		int index_table_sz = 0;
		
		float * light_ids = nullptr;
		int light_ids_sx = 0;
		int light_ids_sy = 0;
		
		float world_to_volume_scale = 0.f;
		
		void free();
	};

	class LightVolumeBuilder
	{

	private:
	
		enum LightType
		{
			kLightType_Point,
			kLightType_Spot
		};
		
		struct Light
		{
			int id;

			LightType type;
			Vec3 position;
			Vec3 direction;
			float radius;
			float spotAngle;
			float spotFarDistance;
		};
		
		std::vector<Light> lights;

	public:

		void addPointLight(
			const int id,
			Vec3Arg position,
			const float radius);
		void addSpotLight(
			const int id,
			Vec3Arg position,
			Vec3Arg direction,
			const float angle,
			const float farDistance);
		void reset();

		LightVolumeData generateLightVolumeData(
			const int halfResolution,
			const float extents,
			const bool infiniteSpaceMode) const;
		
		static void computeSpotLightAabb(
			Vec3Arg position,
			Vec3Arg direction,
			float angle,
			float farDistance,
			Vec3 & out_min,
			Vec3 & out_max);
		
	};
}
