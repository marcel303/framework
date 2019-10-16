#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterRerange : Filter
	{
		int min = 0;
		int max = 255;

	    virtual bool apply(const Heightfield & src, Heightfield & dst) override;
	    virtual bool setOption(const std::string & name, const char * value) override;
	};
}
