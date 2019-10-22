#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterMapMaximum : Filter
	{
		int max = 255;

		bool setMax(const int max);
		
	    virtual bool apply(const Heightfield & src, Heightfield & dst) override;
	    virtual bool setOption(const std::string & name, const char * value) override;
	};
	
	bool filterMapMaximum(const Heightfield & src, Heightfield & dst, const int max);
}
