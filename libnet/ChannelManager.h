#pragma once

#include <map>
#include <vector>
#include "Channel.h"
#include "ChannelHandler.h"
#include "ChannelTypes.h"
#include "NetHandlePool.h"
#include "NetSocket.h"
#include "PacketListener.h"

class ChannelManager : public PacketListener
{
public:
	ChannelManager();
	~ChannelManager();

	bool Initialize(ChannelHandler * handler, uint16_t serverPort, bool enableServer);
	void Shutdown(bool sendDisconnectNotification);

	Channel * SV_CreateChannel();
	Channel * CL_CreateChannel();
	Channel * CreateChannelEx(ChannelType type, ChannelSide side);
	void DestroyChannel(Channel * channel);
	void DestroyChannelQueued(Channel * channel);

	void Update(uint32_t time);

	virtual void OnReceive(Packet & packet, Channel * channel);

	void HandleTrunk(Packet & packet, Channel * channel);
	void HandleConnect(Packet & packet, Channel * channel);
	void HandleConnectOK(Packet & packet, Channel * channel);
	void HandleConnectError(Packet & packet, Channel * channel);
	void HandleConnectAck(Packet & packet, Channel * channel);
	void HandleDisconnect(Packet & packet, Channel * channel);
	void HandleUnpack(Packet & packet, Channel * channel);

	Channel* FindChannel(uint32_t id);

	typedef std::map<uint32_t, Channel *> ChannelMap;
	typedef ChannelMap::iterator ChannelMapItr;

	SharedNetSocket m_socket;
	Channel * m_listenChannel;
	
	ChannelMap m_channels;
	HandlePool<uint16_t> m_channelIds;

	ChannelHandler * m_handler;

	std::vector<Channel *> m_destroyedChannels;
};
