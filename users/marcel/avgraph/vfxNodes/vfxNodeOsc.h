#pragma once

#include "vfxNodeBase.h"
#include <list>

class MyOscPacketListener;
class UdpListeningReceiveSocket;

struct SDL_Thread;

struct VfxNodeOsc : VfxNodeBase
{
	const int kMaxHistory = 10;
	
	struct HistoryItem
	{
		std::string eventName;
	};
	
	enum Input
	{
		kInput_Port,
		kInput_IpAddress,
		kInput_COUNT
	};
	
	enum Output
	{
		kOutput_Trigger,
		kOutput_COUNT
	};
	
	VfxTriggerData eventId;
	
	MyOscPacketListener * oscPacketListener;
	UdpListeningReceiveSocket * oscReceiveSocket;
	
	SDL_Thread * oscMessageThread;
	
	std::list<HistoryItem> history;
	
	VfxNodeOsc();
	virtual ~VfxNodeOsc() override;
	
	virtual void init(const GraphNode & node) override;
	
	virtual void tick(const float dt) override;
	
	virtual void getDescription(VfxNodeDescription & d) override;
	
	static int executeOscThread(void * data);
};
