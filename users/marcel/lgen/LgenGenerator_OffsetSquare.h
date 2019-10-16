#pragma once

#include "LgenGenerator.h"

namespace lgen
{
	struct Generator_OffsetSquare : Generator
	{
		virtual bool generate(Heighfield * heightfield) override; ///< Generate offset-square heightfield.
	};
}
