#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterRerange : Filter
	{
		int min = 0;
		int max = 255;

	    virtual bool apply(const Generator * src, Generator * dst) override;
	    virtual bool setOption(const std::string & name, char * value) override;
	};
}
