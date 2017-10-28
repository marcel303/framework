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
#include "vfxNodeWekinator.h"

#define OSC_BUFFER_SIZE 1024

VFX_NODE_TYPE(wekinator, VfxNodeWekinator)
{
	typeName = "wekinator";

	in("endpoint", "string");
	in("send", "bool", "1");
	in("sendPath", "string", "/wek/inputs");
	in("recv", "bool", "1");
	in("recvPath", "string", "/wek/outputs");
	in("channels", "channels");
	in("recordBegin", "trigger");
	in("recordEnd", "trigger");
	in("train", "trigger");
	in("runBegin", "trigger");
	in("runEnd", "trigger");
	out("channels", "channels");
}

VfxNodeWekinator::VfxNodeWekinator()
	: VfxNodeBase()
	, channelsOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_EndpointName, kVfxPlugType_String);
	addInput(kInput_SendEnabled, kVfxPlugType_Bool);
	addInput(kInput_SendPath, kVfxPlugType_String);
	addInput(kInput_RecvEnabled, kVfxPlugType_Bool);
	addInput(kInput_RecvPath, kVfxPlugType_String);
	addInput(kInput_Channels, kVfxPlugType_Channels);
	addInput(kInput_RecordBegin, kVfxPlugType_Trigger);
	addInput(kInput_RecordEnd, kVfxPlugType_Trigger);
	addInput(kInput_Train, kVfxPlugType_Trigger);
	addInput(kInput_RunBegin, kVfxPlugType_Trigger);
	addInput(kInput_RunEnd, kVfxPlugType_Trigger);
	addOutput(kOutput_Channels, kVfxPlugType_Channels, &channelsOutput);
}

VfxNodeWekinator::~VfxNodeWekinator()
{
}

void VfxNodeWekinator::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeWekinator);

	if (isPassthrough)
		return;

	const char * endpointName = getInputString(kInput_EndpointName, "");
	const bool sendEnabled = getInputBool(kInput_SendEnabled, true);
	const char * sendPath = getInputString(kInput_SendPath, "/wek/inputs");
	const bool recvEnabled = getInputBool(kInput_RecvEnabled, true);
	
	const VfxChannels * inputChannels = getInputChannels(kInput_Channels, nullptr);

	if (sendEnabled && inputChannels != nullptr)
	{
		OscSender * oscSender = g_oscEndpointMgr.findSender(endpointName);

		if (oscSender != nullptr)
		{
			char buffer[OSC_BUFFER_SIZE];
		
			osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);

			p
				<< osc::BeginBundleImmediate
				<< osc::BeginMessage(sendPath);
			
			if (inputChannels->numChannels >= 1)
			{
				const VfxChannel & channel = inputChannels->channels[0];
				
				for (int i = 0; i < inputChannels->size; ++i)
				{
					const float value = channel.data[i];
					
					p << value;
				}
			}
			
			p
				<< osc::EndMessage
				<< osc::EndBundle;
			
			oscSender->send(p.Data(), p.Size());
		}
	}
	
	if (recvEnabled)
	{
		OscReceiver * oscReceiver = g_oscEndpointMgr.findReceiver(endpointName);
		
		if (oscReceiver != nullptr)
		{
			oscReceiver->pollMessages(this);
		}
	}
}

void VfxNodeWekinator::sendControlMessage(const char * path)
{
	const char * endpointName = getInputString(kInput_EndpointName, "");
	OscSender * oscSender = g_oscEndpointMgr.findSender(endpointName);
	
	if (oscSender != nullptr)
	{
		char buffer[OSC_BUFFER_SIZE];
		
		osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);

		p
			<< osc::BeginBundleImmediate
			<< osc::BeginMessage(path);
		
		p
			<< osc::EndMessage
			<< osc::EndBundle;
		
		oscSender->send(p.Data(), p.Size());
	}
}

void VfxNodeWekinator::handleTrigger(const int index)
{
	if (index == kInput_RecordBegin)
	{
		sendControlMessage("/wekinator/control/startRecording");
	}
	else if (index == kInput_RecordEnd)
	{
		sendControlMessage("/wekinator/control/stopRecording");
	}
	else if (index == kInput_Train)
	{
		sendControlMessage("/wekinator/control/train");
	}
	else if (index == kInput_RunBegin)
	{
		sendControlMessage("/wekinator/control/startRunning");
	}
	else if (index == kInput_RunEnd)
	{
		sendControlMessage("/wekinator/control/stopRunning");
	}
}

void VfxNodeWekinator::handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint)
{
	//LOG_DBG("received message: %s", m.AddressPattern());
	
	const char * recvPath = getInputString(kInput_RecvPath, "/wek/outputs");
	
	if (strcmp(m.AddressPattern(), recvPath) == 0)
	{
		//LOG_DBG("OSC path is a match", 0);
		
		try
		{
			channelData.alloc(m.ArgumentCount());
			
			auto args = m.ArgumentStream();
			
			for (int i = 0; i < channelData.size; ++i)
			{
				args >> channelData.data[i];
			}
			
			channelsOutput.setDataContiguous(channelData.data, false, channelData.size, 1);
		}
		catch (std::exception & e)
		{
			LOG_ERR("failed to read Wekinator output: %s", e.what());
		}
	}
}
