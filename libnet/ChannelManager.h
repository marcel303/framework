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

	bool Initialize(PacketDispatcher * packetDispatcher, ChannelHandler * handler, uint16_t serverPort, bool enableServer, uint32_t serverVersion);
	void Shutdown(bool sendDisconnectNotification);

	bool IsInitialized();
	void SetChannelTimeoutMS(uint32_t timeout);

	Channel * CreateListenChannel(ChannelPool pool);
	Channel * CreateChannel(ChannelPool pool);
	Channel * CreateChannelEx(ChannelType type, ChannelPool pool);
	void DestroyChannel(Channel * channel);
	void DestroyChannelQueued(Channel * channel);

	void Update(uint64_t time);

	virtual void OnReceive(Packet & packet, Channel * channel);

	void HandleTrunk(Packet & packet, Channel * channel);
	void HandleConnect(Packet & packet, Channel * channel);
	void HandleConnectOK(Packet & packet, Channel * channel);
	void HandleConnectError(Packet & packet, Channel * channel);
	void HandleConnectAck(Packet & packet, Channel * channel);
	void HandleDisconnect(Packet & packet, Channel * channel);
	void HandleDisconnectAck(Packet & packet, Channel * channel);
	void HandleUnpack(Packet & packet, Channel * channel);

	Channel* FindChannel(uint32_t id);

	typedef std::map<uint32_t, Channel *> ChannelMap;
	typedef ChannelMap::iterator ChannelMapItr;

	SharedNetSocket m_socket;
	Channel * m_listenChannel;
	
	uint32_t m_serverVersion;
	uint32_t m_channelTimeout;

	ChannelMap m_channels;
	HandlePool<uint16_t> m_channelIds;

	PacketDispatcher * m_packetDispatcher;

	ChannelHandler * m_handler;

	std::vector<Channel *> m_destroyedChannels;
};
