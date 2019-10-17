#pragma once

#include "LgenHeightfield.h"

namespace lgen
{
	struct Generator
	{
		virtual ~Generator();

		virtual bool generate(Heightfield & heightfield) = 0;
	};
}
