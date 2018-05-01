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

#include "oscReceiver.h"

#include "ip/UdpSocket.h"
#include "osc/OscOutboundPacketStream.h"
#include <list>
#include <SDL2/SDL.h>

#include "Debugging.h"
#include "Log.h"

class OscPacketListener : public osc::OscPacketListener
{
public:
	struct ReceivedMessage
	{
		char * data;
		int size;
		IpEndpointName remoteEndpoint;
	};
	
	SDL_mutex * receiveMutex;
	OscReceiveHandler * receiveHandler;
	
	std::list<ReceivedMessage> receivedMessages;
	std::list<ReceivedMessage> receivedMessagesCopy;
	
	OscPacketListener()
		: receiveMutex(nullptr)
		, receiveHandler(nullptr)
		, receivedMessages()
	{
		receiveMutex = SDL_CreateMutex();
	}
	
	~OscPacketListener()
	{
		freeMessages();
		swapMessages();
		freeMessages();
		
		SDL_DestroyMutex(receiveMutex);
		receiveMutex = nullptr;
		
		Assert(receiveHandler == nullptr);
		receiveHandler = nullptr;
	}
	
	void swapMessages()
	{
		SDL_LockMutex(receiveMutex);
		{
			receivedMessagesCopy = receivedMessages;
			receivedMessages.clear();
		}
		SDL_UnlockMutex(receiveMutex);
	}
	
	void freeMessages()
	{
		for (auto & receivedMessage : receivedMessagesCopy)
		{
			delete receivedMessage.data;
			receivedMessage.data = nullptr;
		}
		
		receivedMessagesCopy.clear();
	}
	
	void pollMessages()
	{
		for (auto & receivedMessage : receivedMessagesCopy)
		{
			const osc::ReceivedPacket p(receivedMessage.data, receivedMessage.size);
			const IpEndpointName & remoteEndpoint = receivedMessage.remoteEndpoint;
			
			if (p.IsBundle())
			{
				ProcessBundle(osc::ReceivedBundle(p), remoteEndpoint);
			}
			else
			{
				ProcessMessage(osc::ReceivedMessage(p), remoteEndpoint);
			}
		}
	}
	
	void flushMessages()
	{
		swapMessages();
		pollMessages();
		freeMessages();
	}
	
protected:
	virtual void ProcessPacket(const char * data, int size, const IpEndpointName & remoteEndpoint) override
	{
		ReceivedMessage message;
		message.data = new char[size];
		message.size = size;
		message.remoteEndpoint = remoteEndpoint;
		memcpy(message.data, data, size);
		
		SDL_LockMutex(receiveMutex);
		{
			receivedMessages.push_back(message);
		}
		SDL_UnlockMutex(receiveMutex);
	}
	
	virtual void ProcessBundle(const osc::ReceivedBundle & b, const IpEndpointName & remoteEndpoint) override
	{
		//LOG_DBG("ProcessBundle: timeTag=%llu", b.TimeTag());

		osc::OscPacketListener::ProcessBundle(b, remoteEndpoint);
	}

	virtual void ProcessMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) override
	{
		try
		{
			if (receiveHandler != nullptr)
			{
				receiveHandler->handleOscMessage(m, remoteEndpoint);
			}
		}
		catch (osc::Exception & e)
		{
			LOG_ERR("error while parsing message: %s: %s", m.AddressPattern(), e.what());
		}
	}
};

//

OscReceiver::OscReceiver()
	: packetListener(nullptr)
	, receiveSocket(nullptr)
	, messageThread(nullptr)
	, ipAddress()
	, udpPort(0)
{
}

OscReceiver::~OscReceiver()
{
	shut();
}

bool OscReceiver::init(const char * ipAddress, const int udpPort)
{
	if (doInit(ipAddress, udpPort) == false)
	{
		shut();
		
		return false;
	}
	else
	{
		return true;
	}
}

bool OscReceiver::doInit(const char * _ipAddress, const int _udpPort)
{
	shut();
	
	//
	
	ipAddress = _ipAddress;
	udpPort = _udpPort;
	
	try
	{
		if (isAddressValid(ipAddress.c_str(), udpPort) == false)
		{
			LOG_WRN("invalid OSC bind address: %s:%d", ipAddress.c_str(), udpPort);
			
			return false;
		}
		else
		{
			LOG_DBG("creating OSC UDP receive socket @ %s:%d", ipAddress.c_str(), udpPort);
			
			// create OSC client and listen
			
			packetListener = new OscPacketListener();
			
			// IpEndpointName::ANY_ADDRESS
			
			IpEndpointName endpointName;
			
			if (ipAddress.empty())
				endpointName = IpEndpointName(IpEndpointName::ANY_ADDRESS, udpPort);
			else
				endpointName = IpEndpointName(ipAddress.c_str(), udpPort);
			
			receiveSocket = new UdpListeningReceiveSocket(endpointName, packetListener);
			
			LOG_DBG("creating OSC receive thread", 0);
		
			messageThread = SDL_CreateThread(executeOscThread, "OSC thread", this);
			
			return true;
		}
	}
	catch (std::exception & e)
	{
		LOG_ERR("failed to start OSC receive thread: %s. ipAddress=%s, updPort=%d", e.what(), ipAddress.c_str(), udpPort);
		
		return false;
	}
}

bool OscReceiver::shut()
{
	LOG_DBG("terminating OSC receive thread", 0);
	
	if (receiveSocket != nullptr)
	{
		receiveSocket->AsynchronousBreak();
	}
	
	if (messageThread != nullptr)
	{
		SDL_WaitThread(messageThread, nullptr);
		messageThread = nullptr;
	}
	
	LOG_DBG("terminating OSC receive thread [done]", 0);
	
	LOG_DBG("terminating OSC UDP receive socket", 0);
	
	delete receiveSocket;
	receiveSocket = nullptr;
	
	LOG_DBG("terminating OSC UDP receive socket [done]", 0);
	
	delete packetListener;
	packetListener = nullptr;
	
	return true;
}

bool OscReceiver::isAddressValid(const char * ipAddress, const int udpPort) const
{
	if (ipAddress == nullptr)
		return false;
	if (udpPort == 0)
		return false;
	if (ipAddress[0] != 0)
	{
		int numDots = 0;
		for (int i = 0; ipAddress[i] != 0; ++i)
			if (ipAddress[i] == '.')
				numDots++;
		if (numDots < 3)
			return false;
	}
	
	return true;
}

bool OscReceiver::isAddressChange(const char * _ipAddress, const int _udpPort) const
{
	return _ipAddress != ipAddress || _udpPort != udpPort;
}

void OscReceiver::recvMessages()
{
	if (packetListener != nullptr)
	{
		packetListener->swapMessages();
	}
}

void OscReceiver::freeMessages()
{
	if (packetListener != nullptr)
	{
		packetListener->freeMessages();
	}
}

void OscReceiver::pollMessages(OscReceiveHandler * receiveHandler)
{
	if (packetListener != nullptr)
	{
		Assert(packetListener->receiveHandler == nullptr);
		packetListener->receiveHandler = receiveHandler;
		{
			packetListener->pollMessages();
		}
		packetListener->receiveHandler = nullptr;
	}
}

void OscReceiver::flushMessages(OscReceiveHandler * receiveHandler)
{
	// update network input
	
	if (packetListener != nullptr)
	{
		Assert(packetListener->receiveHandler == nullptr);
		packetListener->receiveHandler = receiveHandler;
		{
			packetListener->flushMessages();
		}
		packetListener->receiveHandler = nullptr;
	}
}

int OscReceiver::executeOscThread(void * data)
{
	OscReceiver * self = (OscReceiver*)data;
	
	self->receiveSocket->Run();
	
	return 0;
}
