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

struct AudioGraphInstance;

struct VfxNodeAudioGraphPoly : VfxNodeBase
{
	enum HistoryType
	{
		kHistoryType_InstanceCreate,
		kHistoryType_InstanceFree
	};
	
	struct HistoryElem
	{
		HistoryType type;
		float value;
	};
	
	static const int kMaxInstances = 256;
	static const int kMaxHistory = 16;
	
	enum Input
	{
		kInput_Filename,
		kInput_Volume,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Voices,
		kOutput_COUNT
	};
	
	AudioGraphInstance * instances[kMaxInstances];
	
	std::string currentFilename;
	
	std::vector<std::string> currentControlValues;
	
	VfxChannelData voicesData;
	VfxChannel voicesOutput;
	
	HistoryElem history[kMaxHistory];
	int historyWritePos;
	int historySize;
	
	VfxNodeAudioGraphPoly();
	virtual ~VfxNodeAudioGraphPoly() override;
	
	void updateDynamicInputs();
	void listChannels(const VfxChannel ** channels, const VfxChannel * volume, int & numChannels, const int maxChannels);
	void addHistoryElem(const HistoryType type, const float value);
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
};
