#pragma once

#if 1

	#include "remotery.h"

	#define vfxCpuTimingBlock(name) rmt_ScopedCPUSample(name)
	#define vfxGpuTimingBlock(name) rmt_ScopedOpenGLSample(name)
	#define vfxSetThreadName(name) rmt_SetCurrentThreadName(name)

#else

	#define vfxCpuTimingBlock(name) do { } while (false)
	#define vfxGpuTimingBlock(name) do { } while (false)
	#define vfxSetThreadName(name) do { } while (false)

#endif
