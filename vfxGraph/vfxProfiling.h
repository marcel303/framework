/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

#define ENABLE_VFXGRAPH_PROFILING 0 // enables Remotery hooks to time node draw/tick
#define ENABLE_VFXGRAPH_CPU_TIMING 1 // captures CPU timing data for node draw/tick
#define ENABLE_VFXGRAPH_GPU_TIMING 0 // captures GPU timing data for node draw

#if ENABLE_VFXGRAPH_PROFILING

	#include "remotery.h"

	#define vfxCpuTimingBlock(name) rmt_ScopedCPUSample(name)
	#define vfxGpuTimingBlock(name) rmt_ScopedOpenGLSample(name)
	#define vfxSetThreadName(name) rmt_SetCurrentThreadName(name)

#else

	#define vfxCpuTimingBlock(name) do { } while (false)
	#define vfxGpuTimingBlock(name) do { } while (false)
	#define vfxSetThreadName(name) do { } while (false)

#endif
