#pragma once

#include "Generator.h"

namespace lgen
{
	struct Generator_OffsetSquare : Generator
	{
		virtual bool generate() override; ///< Generate offset-square heightfield.
	};
}
