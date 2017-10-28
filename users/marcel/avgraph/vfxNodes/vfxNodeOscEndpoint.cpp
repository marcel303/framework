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

#include "oscEndpointMgr.h"
#include "vfxNodeOscEndpoint.h"

VFX_NODE_TYPE(osc_endpoint, VfxNodeOscEndpoint)
{
	typeName = "osc.endpoint";
	
	in("name", "string");
	in("recv", "bool", "1");
	in("recvIpAddress", "string");
	in("recvPort", "int");
	in("send", "bool", "1");
	in("sendIpAddress", "string");
	in("sendPort", "int");
}

VfxNodeOscEndpoint::VfxNodeOscEndpoint()
	: VfxNodeBase()
	, currentRecvName()
	, currentSendName()
	, oscReceiver(nullptr)
	, oscSender(nullptr)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Name, kVfxPlugType_String);
	addInput(kInput_RecvEnabled, kVfxPlugType_Bool);
	addInput(kInput_RecvIpAddress, kVfxPlugType_String);
	addInput(kInput_RecvPort, kVfxPlugType_Int);
	addInput(kInput_SendEnabled, kVfxPlugType_Bool);
	addInput(kInput_SendIpAddress, kVfxPlugType_String);
	addInput(kInput_SendPort, kVfxPlugType_Int);
}

VfxNodeOscEndpoint::~VfxNodeOscEndpoint()
{
	g_oscEndpointMgr.freeReceiver(oscReceiver);
	
	g_oscEndpointMgr.freeSender(oscSender);
}

void VfxNodeOscEndpoint::init(const GraphNode & node)
{
	const char * name = getInputString(kInput_Name, "");
	const bool recvEnabled = getInputBool(kInput_RecvEnabled, true);
	const bool sendEnabled = getInputBool(kInput_SendEnabled, true);
	
	currentRecvName = name;
	currentSendName = name;
	
	if (recvEnabled)
	{
		const char * ipAddress = getInputString(kInput_RecvIpAddress, "");
		const int udpPort = getInputInt(kInput_RecvPort, 0);
		
		oscReceiver = g_oscEndpointMgr.allocReceiver(name, ipAddress, udpPort);
	}
	
	if (sendEnabled)
	{
		const char * ipAddress = getInputString(kInput_SendIpAddress, "");
		const int udpPort = getInputInt(kInput_SendPort, 0);
		
		oscSender = g_oscEndpointMgr.allocSender(name, ipAddress, udpPort);
	}
}

void VfxNodeOscEndpoint::tick(const float dt)
{
	vfxCpuTimingBlock(VfxNodeOscReceive);
	
	const char * name = getInputString(kInput_Name, "");
	const bool recvEnabled = getInputBool(kInput_RecvEnabled, true) && isPassthrough == false;
	const bool sendEnabled = getInputBool(kInput_SendEnabled, true) && isPassthrough == false;
	
	if (recvEnabled)
	{
		const char * ipAddress = getInputString(kInput_RecvIpAddress, "");
		const int udpPort = getInputInt(kInput_RecvPort, 0);
		
		if (name != currentRecvName || oscReceiver == nullptr || oscReceiver->isAddressChange(ipAddress, udpPort))
		{
			g_oscEndpointMgr.freeReceiver(oscReceiver);
			
			//
			
			currentRecvName = name;
			
			oscReceiver = g_oscEndpointMgr.allocReceiver(name, ipAddress, udpPort);
		}
	}
	else
	{
		if (oscReceiver != nullptr)
		{
			g_oscEndpointMgr.freeReceiver(oscReceiver);
		}
	}
	
	//
	
	if (sendEnabled)
	{
		const char * ipAddress = getInputString(kInput_SendIpAddress, "");
		const int udpPort = getInputInt(kInput_SendPort, 0);
		
		if (name != currentSendName || oscSender == nullptr || oscSender->isAddressChange(ipAddress, udpPort))
		{
			g_oscEndpointMgr.freeSender(oscSender);
			
			//
			
			currentSendName = name;
			
			oscSender = g_oscEndpointMgr.allocSender(name, ipAddress, udpPort);
		}
	}
	else
	{
		if (oscSender != nullptr)
		{
			g_oscEndpointMgr.freeSender(oscSender);
		}
	}
}

void VfxNodeOscEndpoint::getDescription(VfxNodeDescription & d)
{
	const bool recvEnabled = getInputBool(kInput_RecvEnabled, true);
	const bool sendEnabled = getInputBool(kInput_SendEnabled, true);
	
	if (recvEnabled)
	{
		const char * ipAddress = getInputString(kInput_RecvIpAddress, "");
		const int udpPort = getInputInt(kInput_RecvPort, 0);
	
		d.add("bind address: %s:%d", ipAddress, udpPort);
	}
	else
	{
		d.add("receive disabled");
	}
	
	if (sendEnabled)
	{
		const char * ipAddress = getInputString(kInput_SendIpAddress, "");
		const int udpPort = getInputInt(kInput_SendPort, 0);
	
		d.add("target address: %s:%d", ipAddress, udpPort);
	}
	else
	{
		d.add("send disabled");
	}
}
