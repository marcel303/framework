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
#include "vfxNodeMemf.h"

VFX_NODE_TYPE(VfxNodeMemf)
{
	typeName = "in.value";
	
	in("name", "string");
	out("value1", "float");
	out("value2", "float");
	out("value3", "float");
	out("value4", "float");
	out("channel", "channel");
}

VfxNodeMemf::VfxNodeMemf()
	: VfxNodeBase()
	, currentName()
	, valueOutput()
	, channelOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Name, kVfxPlugType_String);
	addOutput(kOutput_Value1, kVfxPlugType_Float, &valueOutput[0]);
	addOutput(kOutput_Value2, kVfxPlugType_Float, &valueOutput[1]);
	addOutput(kOutput_Value3, kVfxPlugType_Float, &valueOutput[2]);
	addOutput(kOutput_Value4, kVfxPlugType_Float, &valueOutput[3]);
	addOutput(kOutput_Channel, kVfxPlugType_Channel, &channelOutput);
}

VfxNodeMemf::~VfxNodeMemf()
{
	if (currentName.empty() == false)
	{
		g_currentVfxGraph->memory.unregisterMemf(currentName.c_str());
		currentName.clear();
	}
}

void VfxNodeMemf::tick(const float dt)
{
	const char * name = getInputString(kInput_Name, nullptr);
	
	if (name == nullptr || isPassthrough)
	{
		if (currentName.empty() == false)
		{
			g_currentVfxGraph->memory.unregisterMemf(currentName.c_str());
			currentName.clear();
			
			valueOutput.SetZero();
			
			channelOutput.reset();
		}
		else
		{
			Assert(currentName.empty());
			Assert(valueOutput[0] == 0.f);
		}
	}
	else
	{
		if (name != currentName)
		{
			if (currentName.empty() == false)
			{
				g_currentVfxGraph->memory.unregisterMemf(currentName.c_str());
				currentName.clear();
			}
			
			g_currentVfxGraph->memory.registerMemf(name, 4);
			currentName = name;
		}
		
		if (g_currentVfxGraph->memory.getMemf(name, valueOutput) == false)
		{
			valueOutput.SetZero();
			
			channelOutput.reset();
		}
		else
		{
			channelOutput.setData2D(&valueOutput[0], false, 1, 4);
		}
	}
}

void VfxNodeMemf::init(const GraphNode & node)
{
	const char * name = getInputString(kInput_Name, nullptr);
	
	if (name != nullptr && isPassthrough == false)
	{
		g_currentVfxGraph->memory.registerMemf(name, 4);
		currentName = name;
	}
}
