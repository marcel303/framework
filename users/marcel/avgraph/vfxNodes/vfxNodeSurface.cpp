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
#include "vfxNodeSurface.h"

extern const int GFX_SX;
extern const int GFX_SY;

VFX_NODE_TYPE(surface, VfxNodeSurface)
{
	typeName = "surface";
	
	in("source", "any");
	in("clear", "bool", "1");
	in("clearColor", "color", "fff");
	in("darken", "bool", "0");
	in("darkenColor", "color", "111");
	in("multiply", "bool", "0");
	in("multiplyColor", "color", "eee");
	out("image", "image");
}

VfxNodeSurface::VfxNodeSurface()
	: VfxNodeBase()
	, surface(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_DontCare, kVfxPlugType_DontCare);
	addInput(kInput_Clear, kVfxPlugType_Bool);
	addInput(kInput_ClearColor, kVfxPlugType_Color);
	addInput(kInput_Darken, kVfxPlugType_Bool);
	addInput(kInput_DarkenColor, kVfxPlugType_Color);
	addInput(kInput_Multiply, kVfxPlugType_Bool);
	addInput(kInput_MultiplyColor, kVfxPlugType_Color);
	addOutput(kOutput_Image, kVfxPlugType_Image, &imageOutput);

	surface = new Surface(GFX_SX, GFX_SY, true);

	imageOutput.texture = surface->getTexture();
}

VfxNodeSurface::~VfxNodeSurface()
{
	delete surface;
	surface = nullptr;
}

void VfxNodeSurface::beforeDraw() const
{
	const bool clear = getInputBool(kInput_Clear, true);
	const VfxColor * clearColor = getInputColor(kInput_ClearColor, nullptr);
	const bool darken = getInputBool(kInput_Darken, false);
	const VfxColor * darkenColor = getInputColor(kInput_DarkenColor, nullptr);
	const bool multiply = getInputBool(kInput_Multiply, false);
	const VfxColor * multiplyColor = getInputColor(kInput_MultiplyColor, nullptr);
	
	pushSurface(surface);
	
	if (clear)
	{
		if (clearColor)
			surface->clearf(clearColor->r, clearColor->g, clearColor->b, clearColor->a);
		else
			surface->clear();
	}
	
	if (darken && darkenColor)
	{
		pushBlend(BLEND_SUBTRACT);
		setColorf(darkenColor->r, darkenColor->g, darkenColor->b, darkenColor->a);
		drawRect(0, 0, surface->getWidth(), surface->getHeight());
		popBlend();
	}
	
	if (multiply && multiplyColor)
	{
		pushBlend(BLEND_MUL);
		setColorf(multiplyColor->r, multiplyColor->g, multiplyColor->b, multiplyColor->a);
		drawRect(0, 0, surface->getWidth(), surface->getHeight());
		popBlend();
	}
}

void VfxNodeSurface::afterDraw() const
{
	popSurface();
}
