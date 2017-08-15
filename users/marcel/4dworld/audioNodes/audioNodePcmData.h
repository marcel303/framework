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
#include "soundmix.h" // PcmData

struct AudioNodePcmData : AudioNodeBase
{
	enum Input
	{
		kInput_Filename,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_PcmData,
		kOutput_COUNT
	};
	
	PcmData pcmData;
	
	std::string currentFilename;
	
	AudioNodePcmData()
		: AudioNodeBase()
		, pcmData()
		, currentFilename()
	{
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Filename, kAudioPlugType_String);
		addOutput(kOutput_PcmData, kAudioPlugType_PcmData, &pcmData);
	}
	
	virtual void tick(const float dt) override;
};
