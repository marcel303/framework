#include "vfxNodeOscSend.h"

#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"

#define OSC_BUFFER_SIZE 1024

VfxNodeOscSend::VfxNodeOscSend()
	: VfxNodeBase()
	, transmitSocket(nullptr)
	, history()
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
	transmitSocket = new UdpTransmitSocket(IpEndpointName("127.0.0.1", 1000));
}

void VfxNodeOscSend::handleTrigger(const int inputSocketIndex, const VfxTriggerData & data)
{
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
	
	for (auto & h : history)
		d.add("%s: %d -> %s:%d", h.eventName.c_str(), h.eventId, h.ipAddress.c_str(), h.udpPort);
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
}
