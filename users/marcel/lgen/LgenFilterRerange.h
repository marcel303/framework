#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterRerange : Filter
	{
		int min = 0;
		int max = 255;

		bool setMinMax(const int min, const int max);
		
	    virtual bool apply(const Heightfield & src, Heightfield & dst) override;
	    virtual bool setOption(const std::string & name, const char * value) override;
	};
	
	bool filterRerange(const Heightfield & src, Heightfield & dst, const int min, const int max);
}
