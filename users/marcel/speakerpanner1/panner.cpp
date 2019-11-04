#include "Debugging.h"
#include "panner.h"
#include <math.h>

namespace SpeakerPanning
{
	Panner_Grid::Panner_Grid()
		: Panner(kPannerType_Grid)
	{
	}
	
	Panner_Grid::~Panner_Grid()
	{
		Assert(sources.empty());
	}

	bool Panner_Grid::init(const GridDescription & in_gridDescription)
	{
		// copy grid description
		
		gridDescription = in_gridDescription;
		
		// allocate storage for speaker infos
		
		const int numSpeakers =
			gridDescription.size[0] *
			gridDescription.size[1] *
			gridDescription.size[2];
		
		speakerInfos.resize(numSpeakers);
		
		return true;
	}

	void Panner_Grid::addSource(SpatialSound::Source * source)
	{
		SourceElem elem;
		elem.source = source;

		sources.push_back(elem);
	}

	void Panner_Grid::removeSource(SpatialSound::Source * source)
	{
		for (auto i = sources.begin(); i != sources.end(); ++i)
		{
			if (i->source == source)
			{
				sources.erase(i);
				break;
			}
		}
	}
	
	void Panner_Grid::updatePanning()
	{
		for (auto & elem : sources)
		{
			updatePanning(elem);
		}
		
		// update speaker panning amplitudes
		
		for (auto & speakerInfo : speakerInfos)
		{
			speakerInfo.panningAmplitude = 0.f;
		}
		
		for (auto & elem : sources)
		{
			for (int i = 0; i < 8; ++i)
			{
				auto & panning = elem.panning[i];
				
				if (panning.speakerIndex == -1)
					continue;
				
				auto & speakerInfo = speakerInfos[panning.speakerIndex];
				
				speakerInfo.panningAmplitude += panning.amount;
			}
		}
	}
	
	void Panner_Grid::updatePanning(SourceElem & elem) const
	{
		float position_f[3];
		
		for (int i = 0; i < 3; ++i)
		{
			if (gridDescription.size[i] == 1)
				position_f[i] = .5f;
			else
			{
				position_f[i] = (elem.source->position[i] - gridDescription.min[i]) / (gridDescription.max[i] - gridDescription.min[i]);
				position_f[i] *= gridDescription.size[i] - 1;
			}
		}
		
		int position_a[3];
		int position_b[3];
		
		for (int i = 0; i < 3; ++i)
		{
			position_a[i] = (int)floorf(position_f[i]);
			position_b[i] = position_a[i] + 1;
			
			if (position_a[i] < 0)
				position_a[i] = 0;
			else if (position_a[i] >= gridDescription.size[i])
				position_a[i] = gridDescription.size[i] - 1;
			
			if (position_b[i] < 0)
				position_b[i] = 0;
			else if (position_b[i] >= gridDescription.size[i])
				position_b[i] = gridDescription.size[i] - 1;
		}
		
		float interp_amount_a[3];
		float interp_amount_b[3];
		
		for (int i = 0; i < 3; ++i)
		{
			float interp_amount = position_f[i] - position_a[i];
			
			if (interp_amount < 0.f)
				interp_amount = 0.f;
			else if (interp_amount > 1.f)
				interp_amount = 1.f;
			
			interp_amount_a[i] = 1.f - interp_amount;
			interp_amount_b[i] = interp_amount;
		}
		
	#define AMOUNT(x, y, z) \
		( \
			(x ? interp_amount_b[0] : interp_amount_a[0]) * \
			(y ? interp_amount_b[1] : interp_amount_a[1]) * \
			(z ? interp_amount_b[2] : interp_amount_a[2]) \
		)
	
		int num_panning_elems = 0;
		
		for (int x = 0; x < 2; ++x)
		{
			for (int y = 0; y < 2; ++y)
			{
				for (int z = 0; z < 2; ++z)
				{
					float amount = AMOUNT(x, y, z);
					
					if (applyConstantPowerCurve)
					{
						// apply constant power curve
						amount = sinf(amount * M_PI * .5f);
					}
					
					elem.panning[num_panning_elems].amount = amount;
					elem.panning[num_panning_elems].speakerIndex =
						calculateSpeakerIndex(
							x ? position_b[0] : position_a[0],
							y ? position_b[1] : position_a[1],
							z ? position_b[2] : position_a[2]);
					
					num_panning_elems++;
				}
			}
		}
	}
}
