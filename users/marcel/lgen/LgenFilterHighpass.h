#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterHighpass : Filter
	{
	    virtual bool apply(Lgen * src, Lgen * dst) override;
	    virtual bool setOption(const std::string & name, char * value) override;
	};
}
