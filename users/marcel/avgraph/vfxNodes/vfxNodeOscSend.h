#pragma once

#include "vfxNodeBase.h"
#include <list>

class MyOscPacketListener;
class UdpListeningReceiveSocket;

struct SDL_Thread;

struct VfxNodeOscSend : VfxNodeBase
{
	const int kMaxHistory = 10;
	
	struct HistoryItem
	{
		std::string eventName;
		int eventId;
		std::string ipAddress;
		int udpPort;
	};
	
	enum Input
	{
		kInput_Port,
		kInput_IpAddress,
		kInput_Event,
		kInput_BaseId,
		kInput_Trigger,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_COUNT
	};
	
	class UdpTransmitSocket * transmitSocket;
	
	std::list<HistoryItem> history;
	int numSends;
	
	VfxNodeOscSend();
	virtual ~VfxNodeOscSend() override;
	
	virtual void init(const GraphNode & node) override;
	
	virtual void handleTrigger(const int inputSocketIndex, const VfxTriggerData & data) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
	
	void sendOscEvent(const char * eventName, const int eventId, const char * ipAddress, const int udpPort);
};
