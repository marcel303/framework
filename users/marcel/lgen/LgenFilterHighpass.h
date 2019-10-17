#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterHighpass : Filter
	{
	    virtual bool apply(const Heightfield & src, Heightfield & dst) override;
	    virtual bool setOption(const std::string & name, const char * value) override;
	};
	
	bool filterHighpass(const Heightfield & src, Heightfield & dst);
}
