#pragma once

#include "vfxNodeBase.h"
#include <string>

class MyOscPacketListener;
class UdpListeningReceiveSocket;

struct VfxNodeOsc : VfxNodeBase
{
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
	
	VfxNodeOsc();
	virtual ~VfxNodeOsc();
	
	virtual void init(const GraphNode & node) override;
	
	virtual void tick(const float dt) override;
	
	static int executeOscThread(void * data);
};
