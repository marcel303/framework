#pragma once

#include <stdint.h>
#include "Debugging.h"
#include "Log.h"

#if 1

static inline void NetAssertImpl(const char * function, const uint32_t line, const char * expr, bool result)
{
	if (result == false)
	{
		LOG_ERR("assertion failed: %s:%u: %s\n", function, line, expr);
		//Assert(false);
	}
}

#define NetAssert(x) NetAssertImpl(__FUNCTION__, __LINE__, #x, x)

#else

#endif
