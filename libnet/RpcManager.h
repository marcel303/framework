#pragma once

#include <map>
#include <stdint.h>
#include "libnet_forward.h"
#include "PacketListener.h"

/*

NOTE: RPC method IDs < 128 do not need to be registered and can be used by the application without first registering RPC calls

example:

enum MyRpcIDs // define up to 127 RPC calls
{
	kMyRpcID_MethodA,
	kMyRpcID_MethodB
};

static uint32_t s_methodC = 0;

void main()
{
	g_rpcManager.RegisterWithID(kMyRpcID_MethodA, MethodA);
	g_rpcManager.RegisterWithID(kMyRpcID_MethodB, MethodB);

	s_methodC = g_rpcManager.Register("MethodC", MethodC);

	BitStream bs;
	g_rpcManager.Call(kMyRpcID_MethodA, bs, ..);
	g_rpcManager.Call(s_methodC,        bs, ..);
}

*/

typedef void (*RpcHandler)(Channel * channel, uint32_t method, BitStream & bs);

class RpcManager : public PacketListener
{
	typedef std::map<uint32_t, RpcHandler> RegistrationMap;
	
	ChannelManager * m_channelMgr;
	RegistrationMap m_registrations;

public:
	RpcManager(ChannelManager * channelMgr);
	~RpcManager();

	bool RegisterWithID(uint32_t method, RpcHandler handler);
	uint32_t Register(const char * name, RpcHandler handler);
	void Unregister(uint32_t method, RpcHandler handler);

	void Call(uint32_t method, const BitStream & bs, ChannelPool channelPool, uint16_t * channelId, bool broadcast, bool invokeLocal);

private:
	virtual void OnReceive(Packet & packet, Channel * channel);

	void CallInternal(Channel * channel, uint32_t method, BitStream & bs);
};
