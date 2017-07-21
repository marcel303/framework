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

#pragma once

#include "audioNodeBase.h"

struct Wavefield2D;

struct AudioNodeWavefield2D : AudioNodeBase
{
	enum Input
	{
		kInput_Size,
		kInput_PositionDampening,
		kInput_VelocityDampening,
		kInput_Tension,
		kInput_Wrap,
		kInput_SampleLocationX,
		kInput_SampleLocationY,
		kInput_Trigger,
		kInput_TriggerLocationX,
		kInput_TriggerLocationY,
		kInput_TriggerAmount,
		kInput_Randomize,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Audio,
		kOutput_COUNT
	};
	
	Wavefield2D * wavefield;

	AudioFloat audioOutput;
	
	AudioNodeWavefield2D();
	virtual ~AudioNodeWavefield2D() override;
	
	virtual void draw() override;
	
	virtual void handleTrigger(const int inputSocketIndex, const AudioTriggerData & data) override;
};
