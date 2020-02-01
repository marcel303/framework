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
#include "vfxNodeMems.h"

VFX_NODE_TYPE(VfxNodeMems)
{
	typeName = "in.string";
	
	in("name", "string");
	out("value", "string");
}

VfxNodeMems::VfxNodeMems()
	: VfxNodeBase()
	, currentName()
	, valueOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Name, kVfxPlugType_String);
	addOutput(kOutput_Value, kVfxPlugType_String, &valueOutput);
}

VfxNodeMems::~VfxNodeMems()
{
	if (currentName.empty() == false)
	{
		g_currentVfxGraph->memory.unregisterMems(currentName.c_str());
		currentName.clear();
	}
}

void VfxNodeMems::tick(const float dt)
{
	const char * name = getInputString(kInput_Name, nullptr);
	
	if (name == nullptr || isPassthrough)
	{
		if (currentName.empty() == false)
		{
			g_currentVfxGraph->memory.unregisterMems(currentName.c_str());
			currentName.clear();
			
			valueOutput.clear();
		}
		else
		{
			Assert(currentName.empty());
			Assert(valueOutput.empty());
		}
	}
	else
	{
		if (name != currentName)
		{
			if (currentName.empty() == false)
			{
				g_currentVfxGraph->memory.unregisterMems(currentName.c_str());
				currentName.clear();
			}
			
			g_currentVfxGraph->memory.registerMems(name);
			currentName = name;
		}
		
		if (g_currentVfxGraph->memory.getMems(name, valueOutput) == false)
			valueOutput.clear();
	}
}

void VfxNodeMems::init(const GraphNode & node)
{
	const char * name = getInputString(kInput_Name, nullptr);
	
	if (name != nullptr && isPassthrough == false)
	{
		g_currentVfxGraph->memory.registerMems(name);
		currentName = name;
	}
}
