#include "framework.h"
#include "vfxNodeTouches.h"

// todo : add wentDown, wentUp and moved triggers

VfxNodeTouches::Touch::Touch()
	: id(0)
	, x(0.f)
	, y(0.f)
	, isDangling(false)
{
}

VfxNodeTouches::VfxNodeTouches()
	: touches()
	, numTouches(0)
	, xChannel()
	, yChannel()
	, isDownOutput(0)
	, isUpOutput(0)
	, wentDownOutput(0)
	, wentUpOutput(0)
	, movedOutput(0)
	, channelsOutput()
{
	resizeSockets(0, kOutput_COUNT);
	addOutput(kOutput_WentDownMask, kVfxPlugType_Int, &wentDownOutput);
	addOutput(kOutput_WentUpMask, kVfxPlugType_Int, &wentUpOutput);
	addOutput(kOutput_IsDownMask, kVfxPlugType_Int, &isDownOutput);
	addOutput(kOutput_IsUpMask, kVfxPlugType_Int, &isUpOutput);
	addOutput(kOutput_MovedMask, kVfxPlugType_Int, &movedOutput);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);

	//

	xChannel.alloc(kMaxTouches);
	yChannel.alloc(kMaxTouches);
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
	// todo : write a note here describing the peculiarities of the process here and why it's necessary for consistency

	wentDownOutput = 0;
	wentUpOutput = 0;
	movedOutput = 0;

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

	// update up/down output

	isDownOutput = 0;

	for (int i = 0; i < numTouches; ++i)
		if (touches[i].isDangling == false)
			isDownOutput |= 1 << i;

	isUpOutput = (~isDownOutput) & ((1 << kMaxTouches) - 1);

	// update XY channels

	for (int i = 0; i < numTouches; ++i)
	{
		xChannel.data[i] = touches[i].x;
		yChannel.data[i] = touches[i].y;
	}

	const float * data[] = { xChannel.data, yChannel.data };
	channelsOutput.setData(data, numTouches, 2);

	// free dangling touches

	for (int i = numTouches - 1; i >= 0; --i)
	{
		if (touches[i].isDangling)
			freeTouch(i);
	}
}
