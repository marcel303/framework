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
	
	virtual void tick(const float dt) override;
};
