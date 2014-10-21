#pragma once

#include <map>
#include <stdint.h>
#include "libnet_forward.h"
#include "PacketListener.h"

typedef void (*RpcHandler)(BitStream & bs);

class RpcManager : public PacketListener
{
	typedef std::map<uint32_t, RpcHandler> RegistrationMap;
	
	ChannelManager * m_channelMgr;
	RegistrationMap m_registrations;

public:
	RpcManager(ChannelManager * channelMgr);
	~RpcManager();

	uint32_t Register(const char * name, RpcHandler handler);
	void Unregister(uint32_t method, RpcHandler handler);

	void Call(uint32_t method, const BitStream & bs, ChannelSide channelSide, uint32_t * channelId, bool broadcast, bool invokeLocal);

private:
	virtual void OnReceive(Packet & packet, Channel * channel);

	void CallInternal(uint32_t method, BitStream & bs);
};
