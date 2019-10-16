#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterRerange : Filter
	{
		int min = 0;
		int max = 255;

	    virtual bool apply(const Heighfield & src, Heighfield & dst) override;
	    virtual bool setOption(const std::string & name, const char * value) override;
	};
}
