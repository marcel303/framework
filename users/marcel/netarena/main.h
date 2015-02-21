#pragma once

#include <map>
#include <vector>
#include "ChannelHandler.h"
#include "Debugging.h"
#include "gametypes.h"
#include "libnet_forward.h"
#include "Options.h"

OPTION_EXTERN(bool, g_devMode);
OPTION_EXTERN(bool, g_monkeyMode);
OPTION_EXTERN(bool, g_logCRCs);

class Client;
class Host;
class MainMenu;
class OptionMenu;
struct ParticleSpawnInfo;
struct Player;
struct PlayerInput;
class PlayerInstanceData;
class StatTimerMenu;
class Surface;
class Ui;

namespace NetSessionDiscovery
{
	class Service;
}

class App : public ChannelHandler
{
public:
	struct ClientInfo
	{
		ClientInfo()
		{
		}

		~ClientInfo()
		{
			Assert(players.empty());
		}

		std::vector<PlayerInstanceData*> players;
	};

	bool m_isHost;

	PacketDispatcher * m_packetDispatcher;
	ChannelManager * m_channelMgr;
	RpcManager * m_rpcMgr;

	NetSessionDiscovery::Service * m_discoveryService;
	Ui * m_discoveryUi;

	Host * m_host;
	std::map<Channel*, ClientInfo> m_hostClients;

	std::vector<Client*> m_clients;
	int m_selectedClient;

	std::vector<int> m_freeControllerList;

	MainMenu * m_mainMenu;

	OptionMenu * m_optionMenu;
	bool m_optionMenuIsOpen;

	StatTimerMenu * m_statTimerMenu;
	bool m_statTimerMenuIsOpen;

	struct PlayerToAddOrRemove
	{
		bool add;
		Channel * channel;
		uint8_t characterIndex;
		int playerId;
	};
	std::vector<PlayerToAddOrRemove> m_playersToAddOrRemove;

	//

	Client * findClientByChannel(Channel * channel);
	void processPlayerChanges();
	void broadcastPlayerInputs();

	// ChannelHandler
	virtual void SV_OnChannelConnect(Channel * channel);
	virtual void SV_OnChannelDisconnect(Channel * channel);
	virtual void CL_OnChannelConnect(Channel * channel) { }
	virtual void CL_OnChannelDisconnect(Channel * channel) { }

	// RpcHandler
	static void handleRpc(Channel * channel, uint32_t method, BitStream & bitStream);

public:
	App();
	~App();

	bool init(bool isHost);
	void shutdown();

	void quit();

	bool startHosting();
	void stopHosting();

	bool findGame();
	void leaveGame(Client * client);

	Client * connect(const char * address);
	void disconnectClient(int index);
	void selectClient(int index);
	Client * getSelectedClient();
	bool isSelectedClient(Client * client);
	bool isSelectedClient(Channel * channel);
	bool isSelectedClient(uint16_t channelId);

	bool tick();
	void draw();

	void netAction(Channel * channel, NetAction action, uint8_t param1, uint8_t param2);
	void netSyncGameSim(Channel * channel);
	void netAddPlayer(Channel * channel, uint8_t characterIndex);
	void netAddPlayerBroadcast(Channel * channel, uint16_t owningChannelId, uint8_t index, uint8_t characterIndex);
	void netRemovePlayer(uint8_t index);
	void netRemovePlayerBroadcast(uint8_t index);
	void netSetPlayerInputs(uint16_t channelId, uint8_t playerId, const PlayerInput & input);
	void netSetPlayerInputsBroadcast();
	void netSetPlayerCharacterIndex(uint16_t channelId, uint8_t playerId, uint8_t characterIndex);
	void netBroadcastCharacterIndex(uint8_t playerId, uint8_t characterIndex);
	void netDebugAction(const char * name, const char * param);

	int allocControllerIndex();
	void freeControllerIndex(int index);
	int getControllerAllocationCount() const;

	std::vector<Client*> getClients() const { return m_clients; }
};

extern App * g_app;
extern int g_updateTicks;

extern Surface * g_colorMap;
extern Surface * g_lightMap;
extern Surface * g_finalMap;

void applyLightMap(Surface & colormap, Surface & lightmap, Surface & dest);
