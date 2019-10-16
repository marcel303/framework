#pragma once

#include "LgenFilter.h"

namespace lgen
{
	class FilterMedian : Filter
	{
		int matrixW = 3;
		int matrixH = 3;

	    virtual bool apply(const Generator * src, Generator * dst) override;
	    virtual bool setOption(const std::string & name, char * value) override;
	};
}
