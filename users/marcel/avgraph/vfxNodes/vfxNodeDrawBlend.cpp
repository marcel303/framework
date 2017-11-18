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
#include "vfxNodeDrawBlend.h"

VFX_ENUM_TYPE(drawImageBlendMode)
{
	elem("alpha");
	elem("opaque");
	elem("add");
	elem("sub");
	elem("mul");
	elem("min");
	elem("max");
}

VFX_NODE_TYPE(VfxNodeDrawBlend)
{
	typeName = "draw.blend";
	
	in("any", "draw", "", "draw");
	inEnum("mode", "drawImageBlendMode");
	out("any", "draw", "draw");
}

VfxNodeDrawBlend::VfxNodeDrawBlend()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Any, kVfxPlugType_DontCare);
	addInput(kInput_BlendMode, kVfxPlugType_Int);
	addOutput(kOutput_Any, kVfxPlugType_DontCare, this);
}

void VfxNodeDrawBlend::beforeDraw() const
{
	if (isPassthrough)
		return;
	
	const BlendMode blendMode = (BlendMode)getInputInt(kInput_BlendMode, 0);
	
	switch (blendMode)
	{
	case kBlendMode_Alpha:
		pushBlend(BLEND_ALPHA);
		break;
	case kBlendMode_Opaque:
		pushBlend(BLEND_OPAQUE);
		break;
	case kBlendMode_Add:
		pushBlend(BLEND_ADD);
		break;
	case kBlendMode_Sub:
		pushBlend(BLEND_SUBTRACT);
		break;
	case kBlendMode_Mul:
		pushBlend(BLEND_MUL);
		break;
	case kBlendMode_Min:
		pushBlend(BLEND_MIN);
		break;
	case kBlendMode_Max:
		pushBlend(BLEND_MAX);
		break;
		
	default:
		Assert(false);
		pushBlend(BLEND_ALPHA);
		break;
	}
	
	const COLOR_POST colorPost = blendMode == kBlendMode_Mul ? POST_BLEND_MUL_FIX : POST_NONE;
	
	pushColorPost(colorPost);
}

void VfxNodeDrawBlend::afterDraw() const
{
	if (isPassthrough)
		return;
	
	popColorPost();
	
	popBlend();
}
