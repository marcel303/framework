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
#include "vfxNodeSequence.h"

VFX_NODE_TYPE(VfxNodeSequence)
{
	typeName = "draw.sequence";
	
	in("01", "draw", "", "draw");
	in("02", "draw", "", "draw");
	in("03", "draw", "", "draw");
	in("04", "draw", "", "draw");
	in("05", "draw", "", "draw");
	in("06", "draw", "", "draw");
	in("07", "draw", "", "draw");
	in("08", "draw", "", "draw");
	out("any", "draw", "draw");
}

VfxNodeSequence::VfxNodeSequence()
	: VfxNodeBase()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_1, kVfxPlugType_Draw);
	addInput(kInput_2, kVfxPlugType_Draw);
	addInput(kInput_3, kVfxPlugType_Draw);
	addInput(kInput_4, kVfxPlugType_Draw);
	addInput(kInput_5, kVfxPlugType_Draw);
	addInput(kInput_6, kVfxPlugType_Draw);
	addInput(kInput_7, kVfxPlugType_Draw);
	addInput(kInput_8, kVfxPlugType_Draw);
	addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
	
	flags |= kFlag_CustomTraverseDraw;
}

void VfxNodeSequence::customTraverseDraw(const int traversalId) const
{
	vfxCpuTimingBlock(VfxNodeSequence);
	
	if (isPassthrough)
		return;
	
	for (int i = kInput_1; i <= kInput_8; ++i)
	{
		const VfxPlug * plug = tryGetInput(i);
		
		if (plug && plug->isConnected())
		{
			for (auto & predep : predeps)
			{
				bool isConnectedToPlug = false;
				
				for (auto & output : predep->outputs)
				{
					if (output.mem == plug->mem)
					{
						isConnectedToPlug = true;
						break;
					}
				}
				
				if (isConnectedToPlug)
				{
					if (predep->lastDrawTraversalId != traversalId)
						predep->traverseDraw(traversalId);
					
					break;
				}
			}
		}
	}
}
