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

#include "vfxNodeOscSend.h"

#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

#define OSC_BUFFER_SIZE 1024

VFX_NODE_TYPE(osc_send, VfxNodeOscSend)
{
	typeName = "osc.send";
	
	in("port", "int");
	in("ipAddress", "string");
	in("event", "string");
	in("baseId", "int");
	in("trigger!", "trigger");
}

VfxNodeOscSend::VfxNodeOscSend()
	: VfxNodeBase()
	, transmitSocket(nullptr)
	, history()
	, numSends()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Port, kVfxPlugType_Int);
	addInput(kInput_IpAddress, kVfxPlugType_String);
	addInput(kInput_Event, kVfxPlugType_String);
	addInput(kInput_BaseId, kVfxPlugType_Int);
	addInput(kInput_Trigger, kVfxPlugType_Trigger);
}

VfxNodeOscSend::~VfxNodeOscSend()
{
	delete transmitSocket;
	transmitSocket = nullptr;
}

void VfxNodeOscSend::init(const GraphNode & node)
{
	transmitSocket = new UdpTransmitSocket(IpEndpointName("127.0.0.1", 8000));
}

void VfxNodeOscSend::handleTrigger(const int inputSocketIndex, const VfxTriggerData & data)
{
	vfxCpuTimingBlock(VfxNodeOscSend);
	
	if (inputSocketIndex == kInput_Trigger)
	{
		const char * ipAddress = getInputString(kInput_IpAddress, "");
		const int udpPort = getInputInt(kInput_Port, 0);
		const char * eventName = getInputString(kInput_Event, "");
		const int baseId = getInputInt(kInput_BaseId, 0);
		const int eventId = data.asInt();
		
		sendOscEvent(eventName, baseId + eventId, ipAddress, udpPort);
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
		d.add("%s:%d <- %s: %d", h.ipAddress.c_str(), h.udpPort, h.eventName.c_str(), h.eventId);
}

void VfxNodeOscSend::sendOscEvent(const char * eventName, const int eventId, const char * ipAddress, const int udpPort)
{
	char buffer[OSC_BUFFER_SIZE];
	
	osc::OutboundPacketStream p(buffer, OSC_BUFFER_SIZE);

	p
		<< osc::BeginBundleImmediate
		<< osc::BeginMessage(eventName);

	p << eventId;
	
	p
		<< osc::EndMessage
		<< osc::EndBundle;

	IpEndpointName endpointName(ipAddress, udpPort);

	transmitSocket->SendTo(endpointName, p.Data(), p.Size());
	
	//
	
	HistoryItem historyItem;
	historyItem.eventName = eventName;
	historyItem.eventId = eventId;
	historyItem.ipAddress = ipAddress;
	historyItem.udpPort = udpPort;
	history.push_front(historyItem);
	
	while (history.size() > kMaxHistory)
		history.pop_back();
	
	numSends++;
}
