#pragma once

#include <map>
#include <vector>
#include "ChannelHandler.h"
#include "libnet_forward.h"
#include "libreplication_forward.h"
#include "Options.h"
#include "ReplicationHandler.h"

OPTION_EXTERN(bool, g_devMode);

class Client;
class Host;
class OptionMenu;
class Player;

class App : public ChannelHandler, public ReplicationHandler
{
	struct ClientInfo
	{
		uint32_t replicationId;
		Player * player;
	};

	bool m_isHost;

	PacketDispatcher * m_packetDispatcher;
	ChannelManager * m_channelMgr;
	RpcManager * m_rpcMgr;
	ReplicationManager * m_replicationMgr;

	Host * m_host;
	std::map<Channel*, ClientInfo> m_hostClients;

	std::vector<Client*> m_clients;

	OptionMenu * m_optionMenu;
	bool m_optionsMenuIsOpen;

	//

	Client * findClientByChannel(Channel * channel);

	// ChannelHandler
	virtual void SV_OnChannelConnect(Channel * channel);
	virtual void SV_OnChannelDisconnect(Channel * channel);
	virtual void CL_OnChannelConnect(Channel * channel) { }
	virtual void CL_OnChannelDisconnect(Channel * channel) { }

	// ReplicationHandler
	virtual bool OnReplicationObjectSerializeType(ReplicationClient * client, ReplicationObject * object, BitStream & bitStream);
	virtual bool OnReplicationObjectCreateType(ReplicationClient * client, BitStream & bitStream, ReplicationObject ** out_object);
	virtual void OnReplicationObjectCreated(ReplicationClient * client, ReplicationObject * object);
	virtual void OnReplicationObjectDestroyed(ReplicationClient * client, ReplicationObject * object);

public:
	App();
	~App();

	bool init(bool isHost);
	void shutdown();

	bool tick();
	void draw();

	void netPlaySound(const char * filename, uint8_t volume = 100);
	void netSetPlayerInputs(uint16_t channelId, uint32_t netId, uint16_t buttons);

	ReplicationManager * getReplicationMgr() { return m_replicationMgr; }
};

extern App * g_app;
