#pragma once

#include "Vec3.h"
#include <vector>

namespace SpeakerPanning
{
	struct Source
	{
		Vec3 position;
	};
	
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
		
		virtual ~Panner()
		{
		}
		
		virtual void addSource(Source * source) = 0;
		virtual void removeSource(Source * source) = 0;
		
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

			Source * source = nullptr;
			PanningElem panning[8];
		};
		
		struct SpeakerInfo
		{
			float panningAmplitude = 0.f;
			float vu = 0.f;
		};

		GridDescription gridDescription;

		std::vector<SourceElem> sources;
		
		bool applyConstantPowerCurve = true;
		
		std::vector<SpeakerInfo> speakerInfos;

		Panner_Grid();
		virtual ~Panner_Grid() override;

		bool init(const GridDescription & in_gridDescription);

		virtual void addSource(Source * source) override;
		virtual void removeSource(Source * source) override;

		virtual void updatePanning() override;
		void updatePanning(SourceElem & elem) const;
		
		int calculateSpeakerIndex(const int x, const int y, const int z) const;
		Vec3 calculateSpeakerPosition(const int x, const int y, const int z) const;
		Vec3 calculateSpeakerPosition(const int in_speakerIndex) const;
		
		const SourceElem & getSourceElemForSource(const Source * source) const;
	};
}
