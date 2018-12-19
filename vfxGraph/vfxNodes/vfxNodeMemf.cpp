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

#include "vfxNodeMemf.h"

VFX_NODE_TYPE(VfxNodeMemf)
{
	typeName = "in.value";
	
	in("name", "string");
	out("value1", "float");
	out("value2", "float");
	out("value3", "float");
	out("value4", "float");
}

VfxNodeMemf::VfxNodeMemf()
	: VfxNodeBase()
	, valueOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Name, kVfxPlugType_String);
	addOutput(kOutput_Value1, kVfxPlugType_Float, &valueOutput[0]);
	addOutput(kOutput_Value2, kVfxPlugType_Float, &valueOutput[1]);
	addOutput(kOutput_Value3, kVfxPlugType_Float, &valueOutput[2]);
	addOutput(kOutput_Value4, kVfxPlugType_Float, &valueOutput[3]);
}

void VfxNodeMemf::tick(const float dt)
{
	const char * name = getInputString(kInput_Name, nullptr);
	
	if (name == nullptr)
	{
		valueOutput.SetZero();
	}
	else
	{
		if (g_currentVfxGraph->getMemf(name, valueOutput) == false)
			valueOutput.SetZero();
	}
}