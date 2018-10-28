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
#include "vfxNodes/oscEndpointMgr.h"
#include "vfxNodes/vfxNodeMidiOsc.h"

VFX_NODE_TYPE(VfxNodeMidiOsc)
{
	typeName = "midiosc";

	in("endpoint", "string");
	out("key", "float");
	out("value", "float");
	out("trigger!", "trigger");
}

VfxNodeMidiOsc::VfxNodeMidiOsc()
	: VfxNodeBase()
	, OscReceiveHandler()
	, keyOutput(0.f)
	, valueOutput(0.f)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_OscEndpointName, kVfxPlugType_String);
	addOutput(kOutput_Key, kVfxPlugType_Float, &keyOutput);
	addOutput(kOutput_Value, kVfxPlugType_Float, &valueOutput);
	addOutput(kOutput_Trigger, kVfxPlugType_Trigger, nullptr);
}

void VfxNodeMidiOsc::tick(const float dt)
{
	if (isPassthrough)
		return;
	
	const char * endpointName = getInputString(kInput_OscEndpointName, "");
	
	OscReceiver * oscReceiver = g_oscEndpointMgr.findReceiver(endpointName);
	
	if (oscReceiver != nullptr)
	{
		oscReceiver->pollMessages(this);
	}
}

void VfxNodeMidiOsc::handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint)
{
	//logDebug("received OSC message!");
	
	try
	{
		auto a = m.ArgumentStream();
		
		const char * message;
		
		a >> message;
		
		if (message != nullptr && strcmp(message, "controller_change") == 0)
		{
			osc::int32 key;
			osc::int32 value;
			a >> key;
			a >> value;
			
			keyOutput = key;
			valueOutput = value / 127.f;
			
			trigger(kOutput_Trigger);
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to decode midiosc message: %s", e.what());
	}
}
