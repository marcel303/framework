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
#include "vfxGraph.h"

OscEndpointMgr g_oscEndpointMgr;

OscEndpointMgr::OscEndpointMgr()
	: receivers()
	, senders()
	, lastTraversalId(-1)
{
}

OscReceiver * OscEndpointMgr::allocReceiver(const char * name, const char * ipAddress, const int udpPort)
{
	for (auto & r : receivers)
	{
		if (r.receiver.ipAddress == ipAddress && r.receiver.udpPort == udpPort)
		{
			r.refCount++;
			
			return &r.receiver;
		}
	}
	
	{
		Receiver r;
		
		receivers.push_back(r);
	}
	
	auto & r = receivers.back();
	
	r.name = name;
	
	if (r.receiver.isAddressValid(ipAddress, udpPort))
		r.receiver.init(ipAddress, udpPort);
	
	r.refCount++;
	
	return &r.receiver;
}

void OscEndpointMgr::freeReceiver(OscReceiver *& receiver)
{
	for (auto rItr = receivers.begin(); rItr != receivers.end(); ++rItr)
	{
		auto & r = *rItr;
		
		if (receiver == &r.receiver)
		{
			r.refCount--;
			
			if (r.refCount == 0)
			{
				r.receiver.shut();
				
				receivers.erase(rItr);
				
				break;
			}
		}
	}
}

OscReceiver * OscEndpointMgr::findReceiver(const char * name)
{
	for (auto & r : receivers)
	{
		if (r.name == name)
		{
			return &r.receiver;
		}
	}
	
	return nullptr;
}

OscSender * OscEndpointMgr::allocSender(const char * name, const char * ipAddress, const int udpPort)
{
	for (auto & s : senders)
	{
		if (s.sender.ipAddress == ipAddress && s.sender.udpPort == udpPort)
		{
			if (s.name == name)
			{
				s.refCount++;
				
				return &s.sender;
			}
			else
			{
				LOG_ERR("ipAddress and port conflict for sender with name %s", name);
				
				return nullptr;
			}
		}
	}
	
	{
		Sender s;
		
		senders.push_back(s);
	}
	
	auto & s = senders.back();
	
	s.name = name;
	
	if (s.sender.isAddressValid(ipAddress, udpPort))
		s.sender.init(ipAddress, udpPort);
	
	s.refCount++;
	
	return &s.sender;
}

void OscEndpointMgr::freeSender(OscSender *& sender)
{
	for (auto sItr = senders.begin(); sItr != senders.end(); ++sItr)
	{
		auto & s = *sItr;
		
		if (sender == &s.sender)
		{
			s.refCount--;
			
			if (s.refCount == 0)
			{
				s.sender.shut();
				
				senders.erase(sItr);
				
				break;
			}
		}
	}
}

OscSender * OscEndpointMgr::findSender(const char * name)
{
	for (auto & s : senders)
	{
		if (s.name == name)
		{
			return &s.sender;
		}
	}
	
	return nullptr;
}

void OscEndpointMgr::tick()
{
	for (auto & r : receivers)
	{
		r.receiver.freeMessages();
		r.receiver.recvMessages();
	}
}
