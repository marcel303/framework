#pragma once

#include "osc/OscPacketListener.h"
#include <string>

class OscPacketListener;
class UdpListeningReceiveSocket;

struct SDL_Thread;

struct OscReceiveHandler
{
	virtual void handleOscMessage(const osc::ReceivedMessage & m, const IpEndpointName & remoteEndpoint) = 0;
};

struct OscReceiver
{
	OscPacketListener * packetListener;
	UdpListeningReceiveSocket * receiveSocket;
	
	SDL_Thread * messageThread;
	
	std::string ipAddress;
	int udpPort;
	
	OscReceiver();
	~OscReceiver();
	
	bool init(const char * ipAddress, const int udpPort);
	bool doInit(const char * ipAddress, const int udpPort);
	bool shut();
	
	bool isAddressChange(const char * ipAddress, const int udpPort) const;
	
	void tick(OscReceiveHandler * receiveHandler);

	static int executeOscThread(void * data);
};
