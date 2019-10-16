#pragma once

#include "LgenFilter.h"

namespace lgen
{	
	struct FilterMaximum : Filter
	{
		int matrixW = 3;
		int matrixH = 3;
		
		void setMatrixSize(const int w, const int h);

		virtual bool apply(const Heighfield * src, Heighfield * dst) override;
		virtual bool setOption(const std::string & name, char * value) override;
	};
}
