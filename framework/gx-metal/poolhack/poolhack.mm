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

#include "poolhack.h"
#include <QuartzCore/QuartzCore.h>
#include <assert.h>

// one for begin/endDraw (metal drawable scope; acquire/present),
// and one for begin/endRenderPass (draw command buffer scope)
static const int kMaxPools = 2;

static NSAutoreleasePool * s_pools[kMaxPools] = { };
static int s_poolCount = 0;

void poolhack_begin()
{
	assert(s_poolCount < kMaxPools);
	
	s_pools[s_poolCount++] = [[NSAutoreleasePool alloc] init];
}

void poolhack_end()
{
	assert(s_poolCount > 0);
	
	s_poolCount--;
	
	[s_pools[s_poolCount] release];
	s_pools[s_poolCount] = nullptr;
}
