#pragma once

#include "vfxNodeBase.h"

class MyOscPacketListener;
class UdpListeningReceiveSocket;

struct SDL_Thread;

struct VfxNodeOscSend : VfxNodeBase
{
	enum Input
	{
		kInput_Port,
		kInput_IpAddress,
		kInput_Event,
		kInput_Trigger,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	class UdpTransmitSocket * transmitSocket;
	
	VfxNodeOscSend();
	virtual ~VfxNodeOscSend() override;
	
	virtual void init(const GraphNode & node) override;
	
	virtual void handleTrigger(const int socketIndex) override;
	
	void sendOscEvent(const char * eventName, const int eventId, const char * ipAddress, const int udpPort);
};
