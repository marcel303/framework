/*
	Copyright (C) 2020 Marcel Smit
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

#include "osc/OscPacketListener.h"
#include <functional>
#include <string>

class OscPacketListener;
class UdpListeningReceiveSocket;

struct OscReceiveHandler
{
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) = 0;
};

typedef const std::function<void(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint)> OscReceiveFunction;

struct OscReceiver
{
	OscPacketListener * packetListener;
	UdpListeningReceiveSocket * receiveSocket;
	
	void * messageThreadPtr; // std::thread
	
	std::string ipAddress;
	int udpPort;
	
	OscReceiver();
	~OscReceiver();
	
	bool init(const char * ipAddress, const int udpPort);
	bool doInit(const char * ipAddress, const int udpPort);
	bool shut();
	
	bool isAddressValid(const char * ipAddress, const int udpPort) const;
	bool isAddressChange(const char * ipAddress, const int udpPort) const;
	
	void recvMessages();
	void freeMessages();
	void pollMessages(OscReceiveHandler * receiveHandler);
	
	void flushMessages(OscReceiveHandler * receiveHandler);
	void flushMessages(const OscReceiveFunction & receiveFunction);

	static int executeOscThread(void * data);
};

struct OscFirstReceivedPathLearner : OscReceiveHandler
{
	std::string path;
	
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		if (path.empty())
			path = m.AddressPattern();
	}
};
