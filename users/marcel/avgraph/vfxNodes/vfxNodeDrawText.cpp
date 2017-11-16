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
#include "vfxNodeDrawText.h"

extern const int GFX_SX;
extern const int GFX_SY;

VFX_ENUM_TYPE(drawTextSizeMode)
{
	elem("contain");
	elem("dontScale");
	elem("stretch");
	elem("fitX");
	elem("fitY");
	elem("fill");
}

VFX_NODE_TYPE(VfxNodeDrawText)
{
	typeName = "draw.text";
	
	in("text", "string");
	inEnum("sizeMode", "drawTextSizeMode");
	in("align.x", "float");
	in("align.y", "float");
	in("angle", "float");
	in("color", "color", "ffff");
	in("opacity", "float", "1");
	out("any", "any");
}

VfxNodeDrawText::VfxNodeDrawText()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Text, kVfxPlugType_String);
	addInput(kInput_SizeMode, kVfxPlugType_Int);
	addInput(kInput_AlignX, kVfxPlugType_Float);
	addInput(kInput_AlignY, kVfxPlugType_Float);
	addInput(kInput_Angle, kVfxPlugType_Float);
	addInput(kInput_Color, kVfxPlugType_Color);
	addInput(kInput_Opacity, kVfxPlugType_Float);
	addOutput(kOutput_Any, kVfxPlugType_DontCare, this);
}

void VfxNodeDrawText::draw() const
{
	if (isPassthrough)
		return;

	vfxCpuTimingBlock(VfxNodeDrawText);

	const VfxColor defaultColor(1.f, 1.f, 1.f, 1.f);
	const float fontSize = 12.f;

	const char * text = getInputString(kInput_Text, nullptr);
	const SizeMode sizeMode = (SizeMode)getInputInt(kInput_SizeMode, 0);
	const float alignX = getInputFloat(kInput_AlignX, 0.f);
	const float alignY = getInputFloat(kInput_AlignY, 0.f);
	const float angle = getInputFloat(kInput_Angle, 0.f);
	const VfxColor * color = getInputColor(kInput_Color, &defaultColor);
	const float opacity = getInputFloat(kInput_Opacity, 1.f);
	
	if (text != nullptr)
	{
		vfxGpuTimingBlock(VfxNodeDrawText);

		setFont("calibri.ttf");
		pushFontMode(FONT_SDF);

		float scaleX = 1.f;
		float scaleY = 1.f;

		float textSx;
		float textSy;
		measureText(fontSize, textSx, textSy, "%s", text);
		
		const float fillScaleX = GFX_SX / float(textSx);
		const float fillScaleY = GFX_SY / float(textSy);
		
		if (sizeMode == kSizeMode_Fill)
		{
			const float scale = fmaxf(fillScaleX, fillScaleY);
			
			scaleX = scale;
			scaleY = scale;
		}
		else if (sizeMode == kSizeMode_Contain)
		{
			const float scale = fminf(fillScaleX, fillScaleY);
			
			scaleX = scale;
			scaleY = scale;
		}
		else if (sizeMode == kSizeMode_DontScale)
		{
			scaleX = 1.f;
			scaleY = 1.f;
		}
		else if (sizeMode == kSizeMode_Stretch)
		{
			scaleX = fillScaleX;
			scaleY = fillScaleY;
		}
		else if (sizeMode == kSizeMode_FitX)
		{
			const float scale = fillScaleX;
			
			scaleX = scale;
			scaleY = scale;
		}
		else if (sizeMode == kSizeMode_FitY)
		{
			const float scale = fillScaleY;
			
			scaleX = scale;
			scaleY = scale;
		}
		else
		{
			Assert(false);
		}
		
		const float offsetX = GFX_SX / 2.f;
		const float offsetY = GFX_SY / 2.f;
		
		gxPushMatrix();
		{
			gxTranslatef(offsetX, offsetY, 0.f);
			gxScalef(scaleX, scaleY, 1.f);
			gxRotatef(angle, 0, 0, 1);
			
			setColorf(color->r, color->g, color->b, color->a * opacity);
			drawText(0.f, 0.f, fontSize, alignX, alignY, "%s", text);
		}
		gxPopMatrix();

		popFontMode();
	}
}
