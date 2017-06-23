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

enum PrimtiveType
{
	kPrimtiveType_Cirle,
	kPrimtiveType_Quad,
	kPrimtiveType_TriangleUp,
	kPrimtiveType_TriangleDown,
	kPrimtiveType_HLine,
	kPrimtiveType_VLine
};

VFX_ENUM_TYPE(drawPrimitiveType)
{
	elem("circle", kPrimtiveType_Cirle);
	elem("quad", kPrimtiveType_Quad);
	elem("triangle_up", kPrimtiveType_TriangleUp);
	elem("triangle_down", kPrimtiveType_TriangleDown);
	elem("hline", kPrimtiveType_HLine);
	elem("vline", kPrimtiveType_VLine);
}

VFX_NODE_TYPE(draw_primitive, VfxNodeDrawPrimitive)
{
	typeName = "draw.primitive";
	
	inEnum("type", "drawPrimitiveType");
	in("channels", "channels");
	out("any", "any");
}

VfxNodeDrawPrimitive::VfxNodeDrawPrimitive()
	: VfxNodeBase()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Type, kVfxPlugType_Int);
	addInput(kInput_Channels, kVfxPlugType_Channels);
	addOutput(kOutput_Any, kVfxPlugType_DontCare, nullptr);
}

void VfxNodeDrawPrimitive::draw() const
{
	vfxCpuTimingBlock(VfxNodeDrawPrimitive);
	vfxGpuTimingBlock(VfxNodeDrawPrimitive);
	
	const PrimtiveType type = (PrimtiveType)getInputInt(kInput_Type, kPrimtiveType_Cirle);
	const VfxChannels * channels = getInputChannels(kInput_Channels, nullptr);
	
	if (channels == nullptr)
		return;
	
	setColor(colorWhite);
	
	switch (type)
	{
	case kPrimtiveType_Cirle:
		{
			hqBegin(HQ_FILLED_CIRCLES);
			{
				const int num = channels->size;
				
				for (int i = 0; i < num; ++i)
				{
					const float x = channels->numChannels >= 1 ? channels->channels[0].data[i] : 0.f;
					const float y = channels->numChannels >= 2 ? channels->channels[1].data[i] : 0.f;
					const float r = channels->numChannels >= 3 ? channels->channels[2].data[i] : 1.f;
					
					hqFillCircle(x, y, r);
				}
			}
			hqEnd();
			break;
		}
		
	case kPrimtiveType_Quad:
		{
			hqBegin(HQ_FILLED_RECTS);
			{
				const int num = channels->size;
				
				for (int i = 0; i < num; ++i)
				{
					const float x = channels->numChannels >= 1 ? channels->channels[0].data[i] : 0.f;
					const float y = channels->numChannels >= 2 ? channels->channels[1].data[i] : 0.f;
					const float rx = channels->numChannels >= 3 ? channels->channels[2].data[i] : 1.f;
					const float ry = channels->numChannels >= 4 ? channels->channels[3].data[i] : rx;
					
					hqFillRect(x - rx, y - ry, x + rx, y + ry);
				}
			}
			hqEnd();
			break;
		}
		
	case kPrimtiveType_TriangleUp:
		{
			hqBegin(HQ_FILLED_TRIANGLES);
			{
				const int num = channels->size;
				
				for (int i = 0; i < num; ++i)
				{
					const float x = channels->numChannels >= 1 ? channels->channels[0].data[i] : 0.f;
					const float y = channels->numChannels >= 2 ? channels->channels[1].data[i] : 0.f;
					
					hqFillTriangle(x, y - 1.f, x - 1.f, y + 1.f, x + 1.f, y + 1.f);
				}
			}
			hqEnd();
			break;
		}
		
	case kPrimtiveType_TriangleDown:
		{
			hqBegin(HQ_FILLED_TRIANGLES);
			{
				const int num = channels->size;
				
				for (int i = 0; i < num; ++i)
				{
					const float x = channels->numChannels >= 1 ? channels->channels[0].data[i] : 0.f;
					const float y = channels->numChannels >= 2 ? channels->channels[1].data[i] : 0.f;
					
					hqFillTriangle(x, y + 1.f, x - 1.f, y - 1.f, x + 1.f, y - 1.f);
				}
			}
			hqEnd();
			break;
		}
		
	case kPrimtiveType_HLine:
		{
			hqBegin(HQ_LINES);
			{
				const int num = channels->size;
				
				for (int i = 0; i < num; ++i)
				{
					const float x = channels->numChannels >= 1 ? channels->channels[0].data[i] : 0.f;
					const float y = channels->numChannels >= 2 ? channels->channels[1].data[i] : 0.f;
					const float s1 = channels->numChannels >= 3 ? channels->channels[2].data[i] : 1.f;
					const float s2 = channels->numChannels >= 4 ? channels->channels[3].data[i] : s1;
					
					hqLine(x - 1.f, y, s1, x + 1.f, y, s2);
				}
			}
			hqEnd();
			break;
		}
		
	case kPrimtiveType_VLine:
		{
			hqBegin(HQ_LINES);
			{
				const int num = channels->size;
				
				for (int i = 0; i < num; ++i)
				{
					const float x = channels->numChannels >= 1 ? channels->channels[0].data[i] : 0.f;
					const float y = channels->numChannels >= 2 ? channels->channels[1].data[i] : 0.f;
					const float s1 = channels->numChannels >= 3 ? channels->channels[2].data[i] : 1.f;
					const float s2 = channels->numChannels >= 4 ? channels->channels[3].data[i] : s1;
					
					hqLine(x, y - 1.f, s1, x, y + 1.f, s2);
				}
			}
			hqEnd();
			break;
		}
	}
}
