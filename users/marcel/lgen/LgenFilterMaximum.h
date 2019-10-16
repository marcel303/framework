#pragma once

#include "LgenFilter.h"

namespace lgen
{	
	struct FilterMaximum : Filter
	{
		int matrixW = 3;
		int matrixH = 3;

		virtual bool apply(Lgen * src, Lgen * dst) override;
		virtual bool setOption(const std::string & name, char * value) override;
	};
}
