#pragma once

#include "LgenHeightfield.h"

namespace lgen
{
	struct Generator
	{
		virtual ~Generator();

		virtual bool generate(Heighfield * heightfield) = 0;
	};
}

#include "Generator_DiamondSquare.h"
#include "Generator_OffsetSquare.h"
