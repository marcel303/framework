#pragma once

#include "Lgen.h"

namespace lgen
{
	struct LgenOs : Lgen
	{
		virtual bool generate() override; ///< Generate offset-square heightfield.
	};
}
