/*
	Copyright (C) 2020 Marcel Smit
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

#pragma once

#include "vfxNodeBase.h"

struct DelayLine;

struct VfxNodeDelayLine : VfxNodeBase
{
	const static int kSampleRate = 200;
	
	enum Input
	{
		kInput_Value,
		kInput_MaxDelay,
		kInput_Delay1,
		kInput_Delay2,
		kInput_Delay3,
		kInput_Delay4,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value1,
		kOutput_Value2,
		kOutput_Value3,
		kOutput_Value4,
		kOutput_COUNT
	};
	
	float outputValue[4];
	
	float dtRemaining;
	
	DelayLine * delayLine;
	
	VfxNodeDelayLine();
	virtual ~VfxNodeDelayLine() override;
	
	virtual void tick(const float dt) override;
};
