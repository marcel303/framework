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

#include "Ease.h"
#include "vfxNodeBase.h"
#include <math.h>

VFX_ENUM_TYPE(channelCurveType)
{
	elem("linear");
	elem("pow");
	elem("sine");
	elem("sine2");
	elem("back");
	elem("back2");
	elem("bounce");
	elem("bounce2");
}

struct VfxNodeChannelCurve : VfxNodeBase
{
	enum Type
	{
		kType_Linear,
		kType_Pow,
		kType_Sine,
		kType_Sine2,
		kType_Back,
		kType_Back2,
		kType_Bounce,
		kType_Bounce2
	};

	enum Input
	{
		kInput_Type,
		kInput_Size,
		kInput_Reverse,
		kInput_Min,
		kInput_Max,
		kInput_Param,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Channel,
		kOutput_COUNT
	};
	
	VfxChannelData channelData;
	
	VfxChannel channelOutput;
	
	VfxNodeChannelCurve()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Type, kVfxPlugType_Int);
		addInput(kInput_Size, kVfxPlugType_Float);
		addInput(kInput_Reverse, kVfxPlugType_Bool);
		addInput(kInput_Min, kVfxPlugType_Float);
		addInput(kInput_Max, kVfxPlugType_Float);
		addInput(kInput_Param, kVfxPlugType_Float);
		addOutput(kOutput_Channel, kVfxPlugType_Channel, &channelOutput);
	}
	
	virtual void tick(const float dt) override
	{
		if (isPassthrough)
		{
			channelData.free();
			channelOutput.reset();
			return;
		}
		
		const Type type = (Type)getInputInt(kInput_Type, 0);
		const int size = (int)fmaxf(0.f, ceilf(getInputFloat(kInput_Size, 0.f)));
		const bool reverse = getInputBool(kInput_Reverse, false);
		const float min = getInputFloat(kInput_Min, 0.f);
		const float max = getInputFloat(kInput_Max, 1.f);
		const float param = getInputFloat(kInput_Param, 0.f);
		
		channelData.allocOnSizeChange(size);
		channelOutput.setData(channelData.data, true, size);
		
		EaseType easeType;
		
		if (type == kType_Linear)
			easeType = kEaseType_Linear;
		else if (type == kType_Pow)
			easeType = kEaseType_PowIn;
		else if (type == kType_Sine)
			easeType = kEaseType_SineIn;
		else if (type == kType_Sine2)
			easeType = kEaseType_SineOut;
		else if (type == kType_Back)
			easeType = kEaseType_BackIn;
		else if (type == kType_Back2)
			easeType = kEaseType_BackInOut;
		else if (type == kType_Bounce)
			easeType = kEaseType_BounceIn;
		else if (type == kType_Bounce2)
			easeType = kEaseType_BounceInOut;
		else
		{
			Assert(false);
			easeType = kEaseType_Linear;
		}
		
		for (int i = 0; i < size; ++i)
		{
			const float t = size == 1 ? 0.f : i / float(size - 1);
			
			const float e = EvalEase(reverse ? 1.f - t : t, easeType, param);
			
			const float r = min * (1.f - e) + max * e;
			
			channelData.data[i] = r;
		}
	}
};

VFX_NODE_TYPE(VfxNodeChannelCurve)
{
	typeName = "channel.curve";
	
	inEnum("type", "channelCurveType");
	in("size", "float");
	in("reverse", "bool");
	in("min", "float");
	in("max", "float", "1");
	in("param", "float");
	out("channel", "channel");
}
