#pragma once

#include "Vec3.h"
#include <vector>

#include "Debugging.h" // todo : move to cpp

struct SpatialSound
{
	struct Source
	{
		Vec3 position;
	};
};

namespace SpeakerPanning
{
	struct SpeakerDescription
	{
		Vec3 position;
	};
	
	enum PannerType
	{
		kPannerType_Grid
	};

	struct Panner
	{
		PannerType type;
		
		Panner(const PannerType in_type)
			: type(in_type)
		{
		}
		
		virtual void addSource(SpatialSound::Source * source) = 0;
		virtual void removeSource(SpatialSound::Source * source) = 0;
		
		virtual void updatePanning() = 0;
	};

	struct GridDescription
	{
		int size[3] = { 0, 0, 0 };

		Vec3 min;
		Vec3 max;
	};

	struct Panner_Grid : Panner
	{
		struct SourceElem
		{
			struct PanningElem
			{
				int speakerIndex = -1;
				float amount = 0.f;
			};

			SpatialSound::Source * source = nullptr;
			PanningElem panning[8];
		};

		GridDescription gridDescription;

		std::vector<SourceElem> sources;
		
		bool applyConstantPowerCurve = true;

		Panner_Grid();
		~Panner_Grid();

		bool init(const GridDescription & in_gridDescription);

		virtual void addSource(SpatialSound::Source * source) override;
		virtual void removeSource(SpatialSound::Source * source) override;

		virtual void updatePanning() override;
		void updatePanning(SourceElem & elem) const;

		void generateAudio(float * __restrict samples, const int numSamples, const int numChannels);
		
		int calculateSpeakerIndex(const int x, const int y, const int z) const
		{
			return
				x +
				y * gridDescription.size[0] +
				z * gridDescription.size[0] * gridDescription.size[1];
		}
		
		Vec3 calculateSpeakerPosition(const int x, const int y, const int z) const
		{
			Vec3 result;
			
			const int position[3] = { x, y, z };
			
			for (int i = 0; i < 3; ++i)
			{
				if (gridDescription.size[i] == 1)
				{
					result[i] = (gridDescription.min[i] + gridDescription.max[i]) / 2.f;
				}
				else
				{
					result[i] = gridDescription.min[i] + (gridDescription.max[i] - gridDescription.min[i]) / (gridDescription.size[i] - 1) * position[i];
				}
			}
			
			return result;
		}
		
		Vec3 calculateSpeakerPosition(const int in_speakerIndex) const
		{
			int remainder = in_speakerIndex;
			
			const int x = remainder % gridDescription.size[0];
			remainder /= gridDescription.size[0];
			
			const int y = remainder % gridDescription.size[1];
			remainder /= gridDescription.size[1];
			
			const int z = remainder % gridDescription.size[2];
			remainder /= gridDescription.size[2];
			
			Assert(remainder == 0);
			if (remainder != 0)
			{
				printf("???\n");
			}
			
			return calculateSpeakerPosition(x, y, z);
		}
		
		const SourceElem & getSourceElemForSource(const SpatialSound::Source * source) const
		{
			const SourceElem * result = nullptr;
			
			for (auto & sourceElem : sources)
			{
				if (sourceElem.source == source)
				{
					result = &sourceElem;
					break;
				}
			}
			
			Assert(result != nullptr);
			
			return *result;
		}
	};
}
