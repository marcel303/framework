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
#include "vfxNodeOscSend.h"

#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

#define OSC_BUFFER_SIZE 1024

struct OscSender
{
	std::string ipAddress;
	int udpPort;
	
	UdpTransmitSocket * transmitSocket;
	
	OscSender()
		: ipAddress()
		, udpPort(0)
		, transmitSocket(nullptr)
	{
	}
	
	bool isAddressChange(const char * _ipAddress, const int _udpPort) const
	{
		return _ipAddress != ipAddress || _udpPort != udpPort;
	}
	
	bool init(const char * _ipAddress, const int _udpPort)
	{
		try
		{
			shut();
			
			//
			
			ipAddress = _ipAddress;
			udpPort = _udpPort;
			
			transmitSocket = new UdpTransmitSocket(IpEndpointName(ipAddress.c_str(), udpPort));
			
			return true;
		}
		catch (std::exception & e)
		{
			LOG_ERR("failed to init OSC sender: %s", e.what());
			
			shut();
			
			return false;
		}
	}
	
	bool shut()
	{
		try
		{
			if (transmitSocket != nullptr)
			{
				delete transmitSocket;
				transmitSocket = nullptr;
			}
			
			return true;
		}
		catch (std::exception & e)
		{
			LOG_ERR("failed to shut down OSC sender: %s", e.what());
			
			return false;
		}
	}
};

//

VFX_ENUM_TYPE(oscSendMode)
{
	elem("onTick");
	elem("onChange");
	elem("onTrigger");
}

VFX_NODE_TYPE(osc_send, VfxNodeOscSend)
{
	typeName = "osc.send";
	
	in("ipAddress", "string");
	in("port", "int");
	inEnum("sendMode", "oscSendMode");
	in("path", "string");
	in("value", "float");
	in("trigger!", "trigger");
}

VfxNodeOscSend::VfxNodeOscSend()
	: VfxNodeBase()
	, oscSender(nullptr)
	, hasLastSentValue(false)
	, lastSentValue(0.f)
	, history()
	, numSends()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_IpAddress, kVfxPlugType_String);
	addInput(kInput_Port, kVfxPlugType_Int);
	addInput(kInput_SendMode, kVfxPlugType_Int);
	addInput(kInput_Path, kVfxPlugType_String);
	addInput(kInput_Value, kVfxPlugType_Float);
	addInput(kInput_Trigger, kVfxPlugType_Trigger);
}

VfxNodeOscSend::~VfxNodeOscSend()
{
	oscSender->shut();
	
	delete oscSender;
	oscSender = nullptr;
}

void VfxNodeOscSend::init(const GraphNode & node)
{
	const char * ipAddress = getInputString(kInput_IpAddress, "");
	const int udpPort = getInputInt(kInput_Port, 0);
	
	oscSender = new OscSender();
	oscSender->init(ipAddress, udpPort);
}

void VfxNodeOscSend::tick(const float dt)
{
	const char * ipAddress = getInputString(kInput_IpAddress, "");
	const int udpPort = getInputInt(kInput_Port, 0);
	const SendMode sendMode = (SendMode)getInputInt(kInput_SendMode, kSend_OnTick);
	const char * path = getInputString(kInput_Path, nullptr);
	const float value = getInputFloat(kInput_Value, 0.f);
	
	if (oscSender->isAddressChange(ipAddress, udpPort))
	{
		oscSender->shut();
		
		oscSender->init(ipAddress, udpPort);
	}
	
	if (sendMode == kSend_OnTick)
	{
		sendValue(path, value);
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
	const char * ipAddress = getInputString(kInput_IpAddress, "");
	const int udpPort = getInputInt(kInput_Port, 0);
	
	d.add("target: %s:%d", ipAddress, udpPort);
	d.newline();
	
	d.add("sent messages: %d:", numSends);
	for (auto & h : history)
		d.add("%s:%d <- %s: %.6f", h.ipAddress.c_str(), h.udpPort, h.path.c_str(), h.value);
}

void VfxNodeOscSend::sendValue(const char * path, const float value)
{
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
		
		oscSender->transmitSocket->Send(p.Data(), p.Size());
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to send OSC message: %s", e.what());
	}
	
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
