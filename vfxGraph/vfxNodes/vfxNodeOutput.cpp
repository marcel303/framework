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

#include "vfxGraph.h"
#include "vfxNodeOutput.h"

VFX_NODE_TYPE(VfxNodeOutput)
{
	typeName = "output";
	
	in("name", "string");
	in("value", "float");
}

VfxNodeOutput::VfxNodeOutput()
	: value(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Name, kVfxPlugType_String);
	addInput(kInput_Value, kVfxPlugType_Float);

	// register as output node with vfx graph
	
	g_currentVfxGraph->outputNodes.insert(this);
}

VfxNodeOutput::~VfxNodeOutput()
{
	// unregister as output node from vfx graph
	
	Assert(g_currentVfxGraph->outputNodes.count(this) != 0);
	g_currentVfxGraph->outputNodes.erase(this);
}

void VfxNodeOutput::tick(const float dt)
{
	if (isPassthrough)
	{
		name.clear();
		value = 0.f;
		
		return;
	}
	
	name = getInputString(kInput_Name, "");
	value = getInputFloat(kInput_Value, 0.f);
}
