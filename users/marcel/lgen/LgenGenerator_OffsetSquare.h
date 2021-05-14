#pragma once

#include "LgenGenerator.h"

namespace lgen
{
	struct Generator_OffsetSquare : Generator
	{
		virtual bool generate(Heightfield & heightfield, const uint32_t seed) override; ///< Generate offset-square heightfield.
	};
}
