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

#include "framework.h"
#include "vfxNodeTouches.h"

VFX_NODE_TYPE(VfxNodeTouches)
{
	typeName = "touches";
	
	out("isDown", "int");
	out("isUp", "int");
	out("wentDown", "int");
	out("wentUp", "int");
	out("moved", "int");
	out("numTouches", "float");
	out("id", "channel");
	out("x", "channel");
	out("y", "channel");
}

VfxNodeTouches::Touch::Touch()
	: id(0)
	, x(0.f)
	, y(0.f)
	, isDangling(false)
{
}

VfxNodeTouches::VfxNodeTouches()
	: VfxNodeBase()
	, touches()
	, numTouches(0)
	, idChannelData()
	, xChannelData()
	, yChannelData()
	, isDownOutput(0)
	, isUpOutput(0)
	, wentDownOutput(0)
	, wentUpOutput(0)
	, movedOutput(0)
	, numTouchesOutput(0)
	, idChannelOutput()
	, xChannelOutput()
	, yChannelOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addOutput(kOutput_WentDownMask, kVfxPlugType_Int, &wentDownOutput);
	addOutput(kOutput_WentUpMask, kVfxPlugType_Int, &wentUpOutput);
	addOutput(kOutput_IsDownMask, kVfxPlugType_Int, &isDownOutput);
	addOutput(kOutput_IsUpMask, kVfxPlugType_Int, &isUpOutput);
	addOutput(kOutput_MovedMask, kVfxPlugType_Int, &movedOutput);
	addOutput(kOutput_NumTouches, kVfxPlugType_Float, &numTouchesOutput);
	addOutput(kOutput_Id, kVfxPlugType_Channel, &idChannelOutput);
	addOutput(kOutput_X, kVfxPlugType_Channel, &xChannelOutput);
	addOutput(kOutput_Y, kVfxPlugType_Channel, &yChannelOutput);
}

VfxNodeTouches::Touch * VfxNodeTouches::findTouch(const uint64_t id)
{
	for (int i = 0; i < numTouches; ++i)
	{
		if (touches[i].id == id)
			return &touches[i];
	}

	return nullptr;
}

void VfxNodeTouches::freeTouch(const int index)
{
	Assert(index >= 0 && index < numTouches);

	for (int i = index; i < numTouches - 1; ++i)
		touches[i] = touches[i + 1];

	touches[numTouches - 1] = Touch();
	
	numTouches--;
}

VfxNodeTouches::Touch * VfxNodeTouches::allocTouch()
{
	if (numTouches == kMaxTouches)
		return nullptr;
	else
		return &touches[numTouches++];
}

void VfxNodeTouches::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeTouches);
	
	// todo : write a note here describing the peculiarities of the process here and why it's necessary for consistency

	wentDownOutput = 0;
	wentUpOutput = 0;
	movedOutput = 0;

	if (framework.windowIsActive == false)
	{
		for (int i = 0; i < numTouches; ++i)
		{
			touches[i].isDangling = true;
			
			wentUpOutput |= 1 << i;
		}
	}
	else
	{
		for (auto & event : framework.events)
		{
			if (event.type == SDL_FINGERDOWN)
			{
				Touch * touch = findTouch(event.tfinger.fingerId);
				Assert(touch == nullptr);

				if (touch == nullptr)
					touch = allocTouch();
				
				if (touch != nullptr)
				{
					const int touchIndex = touch - touches;

					touch->id = event.tfinger.fingerId;
					touch->x = event.tfinger.x;
					touch->y = event.tfinger.y;

					wentDownOutput |= 1 << touchIndex;
				}
			}
			else if (event.type == SDL_FINGERUP)
			{
				Touch * touch = findTouch(event.tfinger.fingerId);
				
				if (touch != nullptr)
				{
					const int touchIndex = touch - touches;

					touch->x = event.tfinger.x;
					touch->y = event.tfinger.y;
					touch->isDangling = true;

					wentUpOutput |= 1 << touchIndex;
				}
			}
			else if (event.type == SDL_FINGERMOTION)
			{
				Touch * touch = findTouch(event.tfinger.fingerId);
				
				if (touch != nullptr)
				{
					const int touchIndex = touch - touches;

					touch->x = event.tfinger.x;
					touch->y = event.tfinger.y;

					movedOutput |= 1 << touchIndex;
				}
			}
		}
	}
	
	// update up/down output

	isDownOutput = 0;

	for (int i = 0; i < numTouches; ++i)
		if (touches[i].isDangling == false)
			isDownOutput |= 1 << i;

	isUpOutput = (~isDownOutput) & ((1 << kMaxTouches) - 1);
	
	numTouchesOutput = numTouches;
	
	// update XY channels

	for (int i = 0; i < numTouches; ++i)
	{
		// todo : add a more persistent/consistent touch ID
		idChannelData[i] = i;
		xChannelData[i] = touches[i].x;
		yChannelData[i] = touches[i].y;
	}
	
	idChannelOutput.setData(idChannelData, false, numTouches);
	xChannelOutput.setData(xChannelData, false, numTouches);
	yChannelOutput.setData(yChannelData, false, numTouches);

	// free dangling touches

	for (int i = numTouches - 1; i >= 0; --i)
	{
		if (touches[i].isDangling)
			freeTouch(i);
	}
}

void VfxNodeTouches::getDescription(VfxNodeDescription & d)
{
	d.add("touches:");
	
	for (int i = 0; i < numTouches; ++i)
	{
		d.add("[%02d] fingerId: %llx, x: %.2f, y: %.2f", i, touches[i].id, touches[i].x, touches[i].y);
	}
}
