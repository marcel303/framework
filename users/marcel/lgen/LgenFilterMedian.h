#pragma once

#include "LgenFilter.h"

namespace lgen
{
	struct FilterMedian : Filter
	{
		int matrixW = 3;
		int matrixH = 3;

		bool setMatrixSize(const int w, const int h);
		
	    virtual bool apply(const Heightfield & src, Heightfield & dst) override;
	    virtual bool setOption(const std::string & name, const char * value) override;
	};
	
	bool filterMedian(const Heightfield & src, Heightfield & dst, const int matrixSx, const int matrixSy);
}
