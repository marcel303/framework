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
#include "audioTypes.h"

struct AudioResource_Wavefield1D;
struct Wavefield1D;

struct AudioNodeWavefield1D : AudioNodeBase
{
	enum Input
	{
		kInput_Size,
		kInput_Gain,
		kInput_PositionDampening,
		kInput_VelocityDampening,
		kInput_Tension,
		kInput_Wrap,
		kInput_SampleLocation,
		kInput_Trigger,
		kInput_TriggerLocation,
		kInput_TriggerAmount,
		kInput_TriggerSize,
		kInput_Randomize,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Audio,
		kOutput_COUNT
	};
	
	AudioResource_Wavefield1D * wavefieldData;
	int currentDataVersion;
	
	Wavefield1D * wavefield;
	
	AudioRNG rng;
	
	AudioFloat audioOutput;
	
	AudioNodeWavefield1D();
	virtual ~AudioNodeWavefield1D() override;
	
	void randomize();
	
	virtual void init(const GraphNode & node) override;
	
	virtual void tick(const float dt) override;
	
	virtual void handleTrigger(const int inputSocketIndex) override;
};
