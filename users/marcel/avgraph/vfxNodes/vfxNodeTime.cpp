#include "framework.h"
#include "vfxNodeTime.h"

VfxNodeTime::VfxNodeTime()
	: VfxNodeBase()
	, time(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addOutput(kOutput_Time, kVfxPlugType_Float, &time);
}

void VfxNodeTime::tick(const float dt)
{
	// todo : use a synchronized clock
	
	time = framework.time;
}
