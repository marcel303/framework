#pragma once

#include "LgenFilter.h"

namespace lgen
{	
	struct FilterMean : Filter
	{
		int matrixW = 3;
		int matrixH = 3;

	    virtual bool apply(const Lgen * src, Lgen * dst) override;
	    virtual bool setOption(const std::string & name, char * value) override;
	};
}
