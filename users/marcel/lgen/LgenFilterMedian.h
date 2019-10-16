#pragma once

#include "LgenFilter.h"

namespace lgen
{
	class FilterMedian : Filter
	{
		int matrixW = 3;
		int matrixH = 3;

	    virtual bool apply(const Heighfield * src, Heighfield * dst) override;
	    virtual bool setOption(const std::string & name, char * value) override;
	};
}
