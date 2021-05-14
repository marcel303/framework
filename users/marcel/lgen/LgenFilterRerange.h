#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterRerange : Filter
	{
		float min = 0.f;
		float max = 1.f;

		bool setMinMax(const float min, const float max);
		
	    virtual bool apply(const Heightfield & src, Heightfield & dst) override;
	    virtual bool setOption(const std::string & name, const char * value) override;
	};
	
	bool filterRerange(const Heightfield & src, Heightfield & dst, const float min, const float max);
}
