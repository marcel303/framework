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
#include "oscSender.h"
#include <map>
#include <list>

struct OscEndpointMgr : OscReceiveHandler
{
	struct Receiver
	{
		std::string name;
		
		OscReceiver receiver;
		int refCount;
		
		Receiver()
			: name()
			, receiver()
			, refCount(0)
		{
		}
	};
	
	struct Sender
	{
		std::string name;
		
		OscSender sender;
		int refCount;
		
		Sender()
			: name()
			, sender()
			, refCount(0)
		{
		}
	};
	
	std::list<Receiver> receivers;
	std::map<std::string, std::vector<float>> receivedValues;
	int lastTraversalId;
	
	std::list<Sender> senders;
	
	OscEndpointMgr();
	
	OscReceiver * allocReceiver(const char * name, const char * ipAddress, const int udpPort);
	void freeReceiver(OscReceiver *& receiver);
	OscReceiver * findReceiver(const char * name);
	
	OscSender * allocSender(const char * name, const char * ipAddress, const int udpPort);
	void freeSender(OscSender *& sender);
	OscSender * findSender(const char * name);
	
	void tick();
	
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override;
};

extern OscEndpointMgr g_oscEndpointMgr;
