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

#include "vfxNodeBase.h"

struct PhaseHelper
{
	float phase;
	bool isStarted;
	
	PhaseHelper(const float _phase)
		: phase(_phase)
		, isStarted(false)
	{
	}
	
	void start(const float _phase)
	{
		Assert(!isStarted);
		phase = _phase;
		isStarted = true;
	}
	
	void stop(const float _phase)
	{
		Assert(isStarted);
		phase = _phase;
		isStarted = false;
	}
	
	void increment(const float deltaPhase)
	{
		Assert(isStarted);
		phase = std::fmod(phase + deltaPhase, 1.f);
	}
};

struct VfxNodeOscSine : VfxNodeBase
{
	enum Input
	{
		kInput_Frequency,
		kInput_Phase,
		kInput_Restart,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	PhaseHelper phaseHelper;
	
	float outputValue;
	
	VfxNodeOscSine();
	
	virtual void tick(const float dt) override;
};

struct VfxNodeOscSaw : VfxNodeBase
{
	enum Input
	{
		kInput_Frequency,
		kInput_Phase,
		kInput_Restart,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	PhaseHelper phaseHelper;
	
	float outputValue;
	
	VfxNodeOscSaw();
	
	virtual void tick(const float dt) override;
};

struct VfxNodeOscTriangle : VfxNodeBase
{
	enum Input
	{
		kInput_Frequency,
		kInput_Phase,
		kInput_Restart,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	PhaseHelper phaseHelper;
	
	float outputValue;
	
	VfxNodeOscTriangle();
	
	virtual void tick(const float dt) override;
};

struct VfxNodeOscSquare : VfxNodeBase
{
	enum Input
	{
		kInput_Frequency,
		kInput_Phase,
		kInput_Restart,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	PhaseHelper phaseHelper;
	
	float outputValue;
	
	VfxNodeOscSquare();
	
	virtual void tick(const float dt) override;
};

struct VfxNodeOscRandom : VfxNodeBase
{
	enum Input
	{
		kInput_Frequency,
		kInput_Next,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Value,
		kOutput_COUNT
	};
	
	float phase;
	
	float outputValue;
	
	VfxNodeOscRandom();
	
	void next();
	
	virtual void tick(const float dt) override;
	virtual void handleTrigger(const int socketIndex) override;
};
