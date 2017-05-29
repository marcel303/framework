#pragma once

#include "vfxNodeBase.h"

// note : isDown, wentDown and wentUp masks can be routed to a int to binary node and inspected per touch

struct VfxNodeTouches : VfxNodeBase
{
	static const int kMaxTouches = 10;

	struct Touch
	{
		uint64_t id;
		float x;
		float y;

		bool isDangling;

		Touch();
	};

	enum Output
	{
		kOutput_IsDownMask,
		kOutput_IsUpMask,
		kOutput_WentDownMask,
		kOutput_WentUpMask,
		kOutput_MovedMask,
		kOutput_NumTouches,
		kOutput_Channels,
		kOutput_COUNT
	};
	
	Touch touches[kMaxTouches];
	int numTouches;

	VfxChannelData xChannel;
	VfxChannelData yChannel;

	//

	int isDownOutput;
	int isUpOutput;
	int wentDownOutput;
	int wentUpOutput;
	int movedOutput;
	float numTouchesOutput;
	VfxChannels channelsOutput;

	VfxNodeTouches();
	
	Touch * findTouch(const uint64_t id);
	void freeTouch(const int index);
	Touch * allocTouch();

	virtual void tick(const float dt) override;
};
