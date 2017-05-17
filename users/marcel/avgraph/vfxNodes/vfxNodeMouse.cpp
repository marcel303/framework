#include "framework.h"
#include "vfxNodeMouse.h"

VfxNodeMouse::VfxNodeMouse()
	: x(0.f)
	, y(0.f)
	, buttonLeft(0.f)
	, buttonRight(0.f)
{
	resizeSockets(0, kOutput_COUNT);
	addOutput(kOutput_X, kVfxPlugType_Float, &x);
	addOutput(kOutput_Y, kVfxPlugType_Float, &y);
	addOutput(kOutput_ButtonLeft, kVfxPlugType_Float, &buttonLeft);
	addOutput(kOutput_ButtonRight, kVfxPlugType_Float, &buttonRight);
}

void VfxNodeMouse::tick(const float dt)
{
	x = mouse.x / float(framework.windowSx);
	y = mouse.y / float(framework.windowSy);
	buttonLeft = mouse.isDown(BUTTON_LEFT) ? 1.f : 0.f;
	buttonRight = mouse.isDown(BUTTON_RIGHT) ? 1.f : 0.f;
}
