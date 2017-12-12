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

#include "oscReceiver.h"
#include "vfxNodeBase.h"

struct VfxNodeWekinator : VfxNodeBase, OscReceiveHandler
{
	enum Input
	{
		kInput_EndpointName,
		kInput_SendEnabled,
		kInput_SendPath,
		kInput_RecvEnabled,
		kInput_RecvPath,
		kInput_Channel,
		kInput_RecordBegin,
		kInput_RecordEnd,
		kInput_Train,
		kInput_RunBegin,
		kInput_RunEnd,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Outputs,
		kOutput_COUNT
	};
	
	VfxChannelData channelData;
	
	VfxChannel outputsChannel;
	
	VfxNodeWekinator();
	virtual ~VfxNodeWekinator() override;
	
	virtual void tick(const float dt) override;
	
	void sendControlMessage(const char * path);
	virtual void handleTrigger(const int index) override;
	
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override;
};
