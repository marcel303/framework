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

#include "Log.h"
#include "oscEndpointMgr.h"
#include "osc/OscOutboundPacketStream.h"
#include "vfxNodeOscSend.h"

#define OSC_BUFFER_SIZE 1024

VFX_ENUM_TYPE(oscSendMode)
{
	elem("onTick");
	elem("onTimer");
	elem("onChange");
	elem("onTrigger");
}

VFX_NODE_TYPE(VfxNodeOscSend)
{
	typeName = "osc.send";
	
	in("endpoint", "string");
	inEnum("sendMode", "oscSendMode");
	in("path", "string");
	in("interval", "float", "1");
	in("value", "float");
	in("trigger!", "trigger");
}

VfxNodeOscSend::VfxNodeOscSend()
	: VfxNodeBase()
	, hasLastSentValue(false)
	, lastSentValue(0.f)
	, timer(0.f)
	, history()
	, numSends()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_EndpointName, kVfxPlugType_String);
	addInput(kInput_SendMode, kVfxPlugType_Int);
	addInput(kInput_Path, kVfxPlugType_String);
	addInput(kInput_Interval, kVfxPlugType_Float);
	addInput(kInput_Value, kVfxPlugType_Float);
	addInput(kInput_Trigger, kVfxPlugType_Trigger);
}

void VfxNodeOscSend::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeOscSend);
	
	const SendMode sendMode = (SendMode)getInputInt(kInput_SendMode, kSend_OnTick);
	const char * path = getInputString(kInput_Path, nullptr);
	const float interval = getInputFloat(kInput_Interval, 1.f);
	const float value = getInputFloat(kInput_Value, 0.f);
	
	if (sendMode == kSend_OnTick)
	{
		sendValue(path, value);
	}
	else if (sendMode == kSend_OnInterval)
	{
		timer += dt;
			
		if (timer >= interval)
		{
			sendValue(path, value);
			
			timer = 0.f;
		}
	}
	else if (sendMode == kSend_OnChange)
	{
		if (hasLastSentValue == false || value != lastSentValue)
		{
			hasLastSentValue = true;
			lastSentValue = value;
			
			sendValue(path, value);
		}
	}
	
	//
	
	if (sendMode != kSend_OnInterval)
		timer = 0.f;
	if (sendMode != kSend_OnChange)
		hasLastSentValue = false;
}

void VfxNodeOscSend::handleTrigger(const int inputSocketIndex)
{
	vfxCpuTimingBlock(VfxNodeOscSend);
	
	if (inputSocketIndex == kInput_Trigger)
	{
		const SendMode sendMode = (SendMode)getInputInt(kInput_SendMode, kSend_OnTick);
		
		if (sendMode == kSend_OnEvent)
		{
			const char * path = getInputString(kInput_Path, nullptr);
			const float value = getInputFloat(kInput_Value, 0.f);
			
			if (path != nullptr)
			{
				sendValue(path, value);
			}
		}
	}
}

void VfxNodeOscSend::getDescription(VfxNodeDescription & d)
{
	const SendMode sendMode = (SendMode)getInputInt(kInput_SendMode, kSend_OnTick);
	
	if (sendMode == kSend_OnInterval)
	{
		const float interval = getInputFloat(kInput_Interval, 1.f);
		
		d.add("timer: %.2f / %.2f", timer, interval);
	}
	
	//d.add("target: %s", "");
	//d.newline();
	
	d.add("sent messages (%d total):", numSends);
	if (history.empty())
		d.add("(none)");
	for (auto & h : history)
		d.add("%s:%d <- %s: %.6f", h.ipAddress.c_str(), h.udpPort, h.path.c_str(), h.value);
}

void VfxNodeOscSend::sendValue(const char * path, const float value)
{
	if (isPassthrough)
		return;
	if (path == nullptr)
		return;
	
	const char * endpointName = getInputString(kInput_EndpointName, "");
	
	OscSender * oscSender = g_oscEndpointMgr.findSender(endpointName);
	
	if (oscSender == nullptr)
		return;
	
	try
	{
		char buffer[OSC_BUFFER_SIZE];
		
		osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);

		p
			<< osc::BeginBundleImmediate
			<< osc::BeginMessage(path);

		p << value;
		
		p
			<< osc::EndMessage
			<< osc::EndBundle;
		
		oscSender->send(p.Data(), p.Size());
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to send OSC message: %s", e.what());
	}
	
	//
	
	editorIsTriggeredTick = g_currentVfxGraph->currentTickTraversalId;
	
	//
	
	HistoryItem historyItem;
	historyItem.path = path;
	historyItem.value = value;
	historyItem.ipAddress = oscSender->ipAddress;
	historyItem.udpPort = oscSender->udpPort;
	history.push_front(historyItem);
	
	while (history.size() > kMaxHistory)
		history.pop_back();
	
	numSends++;
}
