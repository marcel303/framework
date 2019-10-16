#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterHighpass : Filter
	{
	    virtual bool apply(const Heighfield & src, Heighfield & dst) override;
	    virtual bool setOption(const std::string & name, const char * value) override;
	};
}
