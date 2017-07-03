/*
	Copyright (C) 2017 Marcel Smit
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

#include "framework.h"
#include "vfxGraph.h"
#include "vfxNodeTime.h"

VFX_NODE_TYPE(time, VfxNodeTime)
{
	typeName = "time";
	
	in("scale", "float", "1");
	in("offset", "float");
	out("time", "float");
}

VfxNodeTime::VfxNodeTime()
	: VfxNodeBase()
	, time(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Scale, kVfxPlugType_Float);
	addInput(kInput_Offset, kVfxPlugType_Float);
	addOutput(kOutput_Time, kVfxPlugType_Float, &time);
}

void VfxNodeTime::tick(const float dt)
{
	const float scale = getInputFloat(kInput_Scale, 1.f);
	const float offset = getInputFloat(kInput_Offset, 0.f);

	time = offset + g_currentVfxGraph->time * scale;
}
