#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterMedian : Filter
	{
		int matrixW = 3;
		int matrixH = 3;

		void setMatrixSize(const int w, const int h);
		
	    virtual bool apply(const Heightfield & src, Heightfield & dst) override;
	    virtual bool setOption(const std::string & name, const char * value) override;
	};
}
