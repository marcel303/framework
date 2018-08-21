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

struct JGMOD_PLAYER;

struct VfxNodeJgmod : VfxNodeBase
{
	enum Input
	{
		kInput_Filename,
		kInput_Autoplay,
		kInput_Loop,
		kInput_Gain,
		kInput_Speed,
		kInput_Pitch,
		kInput_Play,
		kInput_Pause,
		kInput_Resume,
		kInput_PrevTrack,
		kInput_NextTrack,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_Track,
		kOutput_Pattern,
		kOutput_Row,
		kOutput_COUNT
	};

	std::string currentFilename;
	
	JGMOD * mod;
	
	JGMOD_PLAYER * player;
	
	float trackOutput;
	float patternOutput;
	float rowOutput;

	VfxNodeJgmod();
	virtual ~VfxNodeJgmod();
	
	virtual void tick(const float dt) override;
	
	virtual void handleTrigger(const int index) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
	
	void load(const char * filename);
	void free();
	
	void play();
};
