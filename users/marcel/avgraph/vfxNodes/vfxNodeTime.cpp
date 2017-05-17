#include "framework.h"
#include "vfxGraph.h"
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
	time = g_currentVfxGraph->time;
}
