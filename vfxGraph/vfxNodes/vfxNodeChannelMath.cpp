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

#include "vfxNodeChannelMath.h"
#include <math.h>

static const float twoPi = M_PI * 2.f;

static float evalMathOp(const float a, const float b, const VfxNodeChannelMath::Type type)
{
	float r = 0.f;
	
	switch (type)
	{
	case VfxNodeChannelMath::kType_Unknown:
		r = 0.f;
		break;
		
	case VfxNodeChannelMath::kType_Add:
		r = a + b;
		break;
	
	case VfxNodeChannelMath::kType_Sub:
		r = a - b;
		break;
		
	case VfxNodeChannelMath::kType_Mul:
		r = a * b;
		break;
		
	case VfxNodeChannelMath::kType_Sin:
		r = sinf(a * twoPi);
		break;
		
	case VfxNodeChannelMath::kType_Cos:
		r = cosf(a * twoPi);
		break;
		
	case VfxNodeChannelMath::kType_Abs:
		r = fabsf(a);
		break;
		
	case VfxNodeChannelMath::kType_Min:
		r = fminf(a, b);
		break;
		
	case VfxNodeChannelMath::kType_Max:
		r = fmaxf(a, b);
		break;
		
	case VfxNodeChannelMath::kType_Sat:
		r = fmaxf(0.f, fminf(1.f, a));
		break;
		
	case VfxNodeChannelMath::kType_Neg:
		r = -a;
		break;
		
	case VfxNodeChannelMath::kType_Sqrt:
		r = sqrtf(a);
		break;
		
	case VfxNodeChannelMath::kType_Pow:
		r = powf(a, b);
		break;
		
	case VfxNodeChannelMath::kType_Exp:
		r = expf(a);
		break;
		
	case VfxNodeChannelMath::kType_Mod:
		r = fmodf(a, b);
		break;
		
	case VfxNodeChannelMath::kType_Fract:
		if (a >= 0.f)
			r = a - floorf(a);
		else
			r = a - ceilf(a);
		break;
		
	case VfxNodeChannelMath::kType_Floor:
		r = floorf(a);
		break;
		
	case VfxNodeChannelMath::kType_Ceil:
		r = ceilf(a);
		break;
		
	case VfxNodeChannelMath::kType_Round:
		r = roundf(a);
		break;
		
	case VfxNodeChannelMath::kType_Sign:
		r = a < 0.f ? -1.f : +1.f;
		break;
		
	case VfxNodeChannelMath::kType_Hypot:
		r = hypotf(a, b);
		break;
	
	case VfxNodeChannelMath::kType_Pitch:
		r = a * powf(2.f, b);
		break;
		
	case VfxNodeChannelMath::kType_Semitone:
		r = a * powf(2.f, b / 12.f);
		break;
	}
	
	return r;
};

//

VFX_ENUM_TYPE(channelMathType)
{
	elem("add", 1);
	elem("sub");
	elem("mul");
	elem("sin");
	elem("cos");
	elem("abs");
	elem("min");
	elem("max");
	elem("sat");
	elem("neg");
	elem("sqrt");
	elem("pow");
	elem("exp");
	elem("mod");
	elem("fract");
	elem("floor");
	elem("ceil");
	elem("round");
	elem("sign");
	elem("hypot");
	elem("pitch");
	elem("semitone");
}

VFX_NODE_TYPE(VfxNodeChannelMath)
{
	typeName = "channel.math";
	
	inEnum("type", "mathType", "1");
	in("a", "channel");
	in("b", "channel");
	out("result", "channel");
}

VfxNodeChannelMath::VfxNodeChannelMath()
	: VfxNodeBase()
	, channelData()
	, channelOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Type, kVfxPlugType_Int);
	addInput(kInput_A, kVfxPlugType_Channel);
	addInput(kInput_B, kVfxPlugType_Channel);
	addOutput(kOutput_R, kVfxPlugType_Channel, &channelOutput);
}

void VfxNodeChannelMath::tick(const float dt)
{
	const Type type = (Type)getInputInt(kInput_Type, kType_Add);
	const VfxChannel * a = getInputChannel(kInput_A, nullptr);
	const VfxChannel * b = getInputChannel(kInput_B, nullptr);
	
	VfxChannelZipper zipper({ a, b });
	
	if (isPassthrough || zipper.done())
	{
		channelData.free();
		channelOutput = *a;
	}
	else if (zipper.done())
	{
		channelData.free();
		channelOutput.reset();
	}
	else
	{
		const int size = zipper.sx * zipper.sy;

		channelData.allocOnSizeChange(size);
		
		channelOutput.size = size;
		
		channelOutput.sx = zipper.sx;
		channelOutput.sy = zipper.sy;
		
		float * __restrict dst = channelData.data;
		
		channelOutput.data = dst;
		channelOutput.continuous = (a && a->continuous) || (b && b->continuous);
		
		while (!zipper.doneY())
		{
			while (!zipper.doneX())
			{
				const float valueA = zipper.read(0, 0.f);
				const float valueB = zipper.read(1, 0.f);

				const float result = evalMathOp(valueA, valueB, type);

				*dst++ = result;
				
				zipper.nextX();
			}
			
			zipper.nextY();
		}
	}
}
