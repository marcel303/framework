#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterMapMaximum : Filter
	{
		float max = 1.f;

		bool setMax(const float max);
		
	    virtual bool apply(const Heightfield & src, Heightfield & dst) override;
	    virtual bool setOption(const std::string & name, const char * value) override;
	};
	
	bool filterMapMaximum(const Heightfield & src, Heightfield & dst, const float max);
}
