#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterHighpass : Filter
	{
	    virtual bool apply(const Generator * src, Generator * dst) override;
	    virtual bool setOption(const std::string & name, char * value) override;
	};
}
