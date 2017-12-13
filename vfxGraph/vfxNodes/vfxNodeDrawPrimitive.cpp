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
#include "vfxNodeDrawPrimitive.h"
#include "vfxTypes.h"

enum PrimitiveType
{
	kPrimitiveType_Cirle,
	kPrimitiveType_Quad,
	kPrimitiveType_TriangleUp,
	kPrimitiveType_TriangleDown,
	kPrimitiveType_HLine,
	kPrimitiveType_VLine,
	kPrimitiveType_RoundedRect
};

VFX_ENUM_TYPE(drawPrimitiveType)
{
	elem("circle", kPrimitiveType_Cirle);
	elem("quad", kPrimitiveType_Quad);
	elem("triangle_up", kPrimitiveType_TriangleUp);
	elem("triangle_down", kPrimitiveType_TriangleDown);
	elem("hline", kPrimitiveType_HLine);
	elem("vline", kPrimitiveType_VLine);
	elem("quad_round", kPrimitiveType_RoundedRect);
}

VFX_NODE_TYPE(VfxNodeDrawPrimitive)
{
	typeName = "draw.primitive";
	
	in("before", "draw");
	inEnum("type", "drawPrimitiveType");
	in("screenSize", "bool");
	in("x", "channel");
	in("y", "channel");
	in("r", "channel");
	in("s", "channel");
	in("size", "float", "1");
	in("fill", "bool", "1");
	in("color", "color", "fff");
	in("image", "image");
	in("stroke", "bool");
	in("strokeSize", "float", "1.0");
	in("strokeColor", "color", "fff");
	out("any", "draw", "draw");
}

VfxNodeDrawPrimitive::VfxNodeDrawPrimitive()
	: VfxNodeBase()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Draw, kVfxPlugType_Draw);
	addInput(kInput_Type, kVfxPlugType_Int);
	addInput(kInput_UseScreenSize, kVfxPlugType_Bool);
	addInput(kInput_XChannel, kVfxPlugType_Channel);
	addInput(kInput_YChannel, kVfxPlugType_Channel);
	addInput(kInput_RChannel, kVfxPlugType_Channel);
	addInput(kInput_SChannel, kVfxPlugType_Channel);
	addInput(kInput_Size, kVfxPlugType_Float);
	addInput(kInput_Fill, kVfxPlugType_Bool);
	addInput(kInput_FillColor, kVfxPlugType_Color);
	addInput(kInput_Image, kVfxPlugType_Image);
	addInput(kInput_Stroke, kVfxPlugType_Bool);
	addInput(kInput_StrokeSize, kVfxPlugType_Float);
	addInput(kInput_StrokeColor, kVfxPlugType_Color);
	addOutput(kOutput_Draw, kVfxPlugType_Draw, this);
}

void VfxNodeDrawPrimitive::draw() const
{
	if (isPassthrough)
		return;
	
	vfxCpuTimingBlock(VfxNodeDrawPrimitive);
	vfxGpuTimingBlock(VfxNodeDrawPrimitive);
	
	const PrimitiveType type = (PrimitiveType)getInputInt(kInput_Type, kPrimitiveType_Cirle);
	const bool useScreenSize = getInputBool(kInput_UseScreenSize, false);
	const VfxChannel * xChannel = getInputChannel(kInput_XChannel, nullptr);
	const VfxChannel * yChannel = getInputChannel(kInput_YChannel, nullptr);
	const VfxChannel * rChannel = getInputChannel(kInput_RChannel, nullptr);
	const VfxChannel * sChannel = getInputChannel(kInput_SChannel, nullptr);
	const float size = getInputFloat(kInput_Size, 1.f);
	const bool fill = getInputBool(kInput_Fill, true);
	const VfxColor * fillColor = getInputColor(kInput_FillColor, nullptr);
	const VfxImageBase * image = getInputImage(kInput_Image, nullptr);
	const bool stroke = getInputBool(kInput_Stroke, false);
	const float strokeSize = getInputFloat(kInput_StrokeSize, 1.f);
	const VfxColor * strokeColor = getInputColor(kInput_StrokeColor, nullptr);
	
	if (fillColor)
		setColorf(fillColor->r, fillColor->g, fillColor->b, fillColor->a);
	else
		setColor(colorWhite);
	
	if (image)
		gxSetTexture(image->getTexture());
	
	if (fill)
	{
		VfxChannelZipper zipper({ xChannel, yChannel, rChannel, sChannel });
		
		switch (type)
		{
		case kPrimitiveType_Cirle:
			{
				hqBegin(HQ_FILLED_CIRCLES, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float r = zipper.read(2, 1.f);
						
						hqFillCircle(x, y, r * size);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
			
		case kPrimitiveType_Quad:
			{
				hqBegin(HQ_FILLED_RECTS, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float rx = zipper.read(2, 1.f);
						const float ry = zipper.read(3, rx);
						
						hqFillRect(x - rx * size, y - ry * size, x + rx * size, y + ry * size);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
			
		case kPrimitiveType_TriangleUp:
			{
				hqBegin(HQ_FILLED_TRIANGLES, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float r = zipper.read(2, 1.f);
						
						hqFillTriangle(x, y - r * size, x - r * size, y + r * size, x + r * size, y + r * size);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
			
		case kPrimitiveType_TriangleDown:
			{
				hqBegin(HQ_FILLED_TRIANGLES, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float r = zipper.read(2, 1.f);
						
						hqFillTriangle(x, y + r * size, x - r * size, y - r * size, x + r * size, y - r * size);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
			
		case kPrimitiveType_HLine:
			{
				hqBegin(HQ_LINES, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float r = zipper.read(2, 1.f);
						const float s = zipper.read(3, 1.f);
						
						hqLine(x - r * size, y, s * strokeSize, x + r * size, y, s * strokeSize);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
			
		case kPrimitiveType_VLine:
			{
				hqBegin(HQ_LINES, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float r = zipper.read(2, 1.f);
						const float s = zipper.read(3, 1.f);
						
						hqLine(x, y - r * size, s * strokeSize, x, y + r * size, s * strokeSize);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
			
		case kPrimitiveType_RoundedRect:
			{
				hqBegin(HQ_FILLED_ROUNDED_RECTS, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float r = zipper.read(2, 1.f);
						const float s = zipper.read(3, 0.f);
						
						hqFillRoundedRect(x - r * size, y - r * size, x + r * size, y + r * size, s * .5f);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
		}
	}
	
	if (stroke)
	{
		VfxChannelZipper zipper({ xChannel, yChannel, rChannel, sChannel });
		
		if (strokeColor)
			setColorf(strokeColor->r, strokeColor->g, strokeColor->b, strokeColor->a);
		else
			setColor(colorWhite);
		
		switch (type)
		{
		case kPrimitiveType_Cirle:
			{
				hqBegin(HQ_STROKED_CIRCLES, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float r = zipper.read(2, 1.f);
						
						hqStrokeCircle(x, y, r * size, strokeSize);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
			
		case kPrimitiveType_Quad:
			{
				hqBegin(HQ_STROKED_RECTS, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float rx = zipper.read(2, 1.f);
						const float ry = zipper.read(3, rx);
						
						hqStrokeRect(x - rx * size, y - ry * size, x + rx * size, y + ry * size, strokeSize);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
			
		case kPrimitiveType_TriangleUp:
			{
				hqBegin(HQ_STROKED_TRIANGLES, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float r = zipper.read(2, 1.f);
						
						hqStrokeTriangle(x, y - r * size, x - r * size, y + r * size, x + r * size, y + r * size, strokeSize);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
			
		case kPrimitiveType_TriangleDown:
			{
				hqBegin(HQ_STROKED_TRIANGLES, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float r = zipper.read(2, 1.f);
						
						hqStrokeTriangle(x, y + r * size, x - r * size, y - r * size, x + r * size, y - r * size, strokeSize);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
			
		case kPrimitiveType_HLine:
		case kPrimitiveType_VLine:
			break;
			
		case kPrimitiveType_RoundedRect:
			{
				hqBegin(HQ_STROKED_ROUNDED_RECTS, useScreenSize);
				{
					while (!zipper.done())
					{
						const float x = zipper.read(0, 0.f);
						const float y = zipper.read(1, 0.f);
						const float r = zipper.read(2, 1.f);
						const float s = zipper.read(3, 0.f);
						
						hqStrokeRoundedRect(x - r * size, y - r * size, x + r * size, y + r * size, s * .5f, strokeSize);
						
						zipper.next();
					}
				}
				hqEnd();
				break;
			}
		}
	}
	
	gxSetTexture(0);
}
