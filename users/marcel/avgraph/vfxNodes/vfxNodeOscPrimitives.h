#pragma once

#include "vfxNodeBase.h"

struct VfxNodeOscSine : VfxNodeBase
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
	float value;
	
	VfxNodeOscSine();
	
	virtual void tick(const float dt) override;
};

struct VfxNodeOscSaw : VfxNodeBase
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
	float value;
	
	VfxNodeOscSaw();
	
	virtual void tick(const float dt) override;
};

struct VfxNodeOscTriangle : VfxNodeBase
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
	float value;
	
	VfxNodeOscTriangle();
	
	virtual void tick(const float dt) override;
};

struct VfxNodeOscSquare : VfxNodeBase
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
	float value;
	
	VfxNodeOscSquare();
	
	virtual void tick(const float dt) override;
};
