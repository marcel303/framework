#pragma once

#include "vfxNodeBase.h"

class MyOscPacketListener;
class UdpListeningReceiveSocket;

struct SDL_Thread;

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
	virtual ~VfxNodeOsc() override;
	
	virtual void init(const GraphNode & node) override;
	
	virtual void tick(const float dt) override;
	
	static int executeOscThread(void * data);
};
