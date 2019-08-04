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

struct VfxNodeOscSheet : VfxNodeBase
{
	enum Input
	{
		kInput_OscEndpoint,
		kInput_OscPrefix,
		kInput_GroupPrefix,
		kInput_OscSheet,
		kInput_SyncOnInit,
		kInput_SendMode,
		kInput_Sync,
		kInput_COUNT
	};

	enum Output
	{
		kOutput_COUNT
	};
	
	enum SendMode
	{
		kSend_OnTick,
		kSend_OnChange,
		kSend_OnSync
	};
	
	struct InputInfo
	{
		std::string oscAddress;
		
		bool isVec2f = false;
		bool isVec3f = false;
		
		float defaultFloat = 0.f;
		int defaultInt = 0;
		bool defaultBool = false;
		
		union
		{
			float lastFloat;
			int lastInt;
			bool lastBool;
		};
	};
	
	std::string currentOscPrefix;
	std::string currentOscSheet;
	bool currentGroupPrefix;
	
	std::vector<InputInfo> inputInfos;
	
	bool sync;
	
	VfxNodeOscSheet();
	
	void updateOscSheet();
	
	virtual void tick(const float dt) override;
	virtual void init(const GraphNode & node) override;
	
	virtual void handleTrigger(const int socketIndex) override;
};
