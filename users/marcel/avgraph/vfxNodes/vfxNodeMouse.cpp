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
#include "vfxNodeMouse.h"

VFX_NODE_TYPE(mouse, VfxNodeMouse)
{
	typeName = "mouse";
	
	out("x", "float");
	out("y", "float");
	out("buttonLeft", "float");
	out("buttonRight", "float");
}

VfxNodeMouse::VfxNodeMouse()
	: VfxNodeBase()
	, x(0.f)
	, y(0.f)
	, buttonLeft(0.f)
	, buttonRight(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
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
