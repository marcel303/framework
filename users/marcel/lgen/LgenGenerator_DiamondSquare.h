#pragma once

#include "LgenGenerator.h"

namespace lgen
{
	struct Generator_DiamondSquare : Generator
	{
		virtual bool generate(Heightfield & heightfield) override;
	};
}
