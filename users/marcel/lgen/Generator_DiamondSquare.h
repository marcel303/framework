#pragma once

#include "Generator.h"

namespace lgen
{
	struct Generator_DiamondSquare : Generator
	{
		virtual bool generate(Heighfield * heightfield) override;
	};
}
