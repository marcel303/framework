#pragma once

#include <map>
#include <vector>
#include "ChannelHandler.h"
#include "config.h"
#include "Debugging.h"
#include "dialog.h" // DialogResult
#include "gametypes.h"
#include "libnet_forward.h"
#include "menu.h"
#include "online.h"
#include "Options.h"

#include <steam_api.h>

OPTION_EXTERN(bool, g_devMode);
OPTION_EXTERN(bool, g_monkeyMode);
OPTION_EXTERN(bool, g_logCRCs);

class Client;
class DialogMgr;
class Host;
class MainMenu;
class MenuMgr;
class OptionMenu;
struct ParticleSpawnInfo;
struct Player;
struct PlayerInput;
struct PlayerInputState;
class PlayerInstanceData;
class StatTimerMenu;
class Surface;
class Ui;

namespace NetSessionDiscovery
{
	class Service;
}

class App : public ChannelHandler, public OnlineCallbacks
{
public:
	enum AppState
	{
		AppState_Offline,
		AppState_Online
	};

	enum NetState
	{
		NetState_Offline,
		NetState_HostCreate,
		NetState_HostDestroy,
		NetState_LobbyCreate,
		NetState_LobbyJoin,
		NetState_LobbyLeave,
		NetState_Online
	};

	struct ClientInfo
	{
		ClientInfo()
		{
		}

		~ClientInfo()
		{
		}
	};

	AppState m_appState;
	NetState m_netState;

	bool m_isHost;
	bool m_isMatchmaking;
	bool m_matchmakingResult;

	PacketDispatcher * m_packetDispatcher;
	ChannelManager * m_channelMgr;
	RpcManager * m_rpcMgr;

#if ENABLE_NETWORKING_DISCOVERY
	NetSessionDiscovery::Service * m_discoveryService;
#endif
#if ENABLE_NETWORKING
	Ui * m_discoveryUi;
#endif

	Host * m_host;
	std::map<uint16_t, ClientInfo> m_hostClients;

	std::vector<Client*> m_clients;
	int m_selectedClient;

	std::vector<int> m_freeControllerList;

	DialogMgr * m_dialogMgr;
	MenuMgr * m_menuMgr;

	UserSettings * m_userSettings;

#if ENABLE_OPTIONS
	OptionMenu * m_optionMenu;
	bool m_optionMenuIsOpen;
#endif

#if ENABLE_OPTIONS
	StatTimerMenu * m_statTimerMenu;
	bool m_statTimerMenuIsOpen;
#endif

	std::string m_displayName;

	//

	Client * findClientByChannel(Channel * channel);
	void broadcastPlayerInputs();

	// ChannelHandler
	virtual void SV_OnChannelConnect(Channel * channel);
	virtual void SV_OnChannelDisconnect(Channel * channel);
	virtual void CL_OnChannelConnect(Channel * channel) { }
	virtual void CL_OnChannelDisconnect(Channel * channel);

	// OnlineCallbacks
	virtual void OnOnlineLobbyCreateResult(OnlineRequestId requestId, bool success);
	virtual void OnOnlineLobbyJoinResult(OnlineRequestId requestId, bool success);
	virtual void OnOnlineLobbyLeaveResult(OnlineRequestId requestId, bool success);

	virtual void OnOnlineLobbyMemberJoined(OnlineLobbyMemberId memberId);
	virtual void OnOnlineLobbyMemberLeft(OnlineLobbyMemberId memberId);

	// RpcHandler
	static void handleRpc(Channel * channel, uint32_t method, BitStream & bitStream);

public:
	App();
	~App();

	bool init();
	void shutdown();

	void setAppState(AppState state);
	void setNetState(NetState state);
	void quit();

	std::string getUserSettingsDirectory();
	std::string getUserSettingsFilename();
	void saveUserSettings();
	void loadUserSettings();

	void startHosting();
	void stopHosting();

	bool pollMatchmaking(bool & isDone, bool & success);

	bool findGame();
	bool joinGame(uint64_t gameId);
	void leaveGame(Client * client);

	Client * connect(const char * address);
	Client * connect(const NetAddress & address);
	void destroyClient(int index);
	void selectClient(int index);
	Client * getSelectedClient();
	bool isSelectedClient(Client * client);
	bool isSelectedClient(Channel * channel);
	bool isSelectedClient(uint16_t channelId);
	void debugSyncGameSims();

	bool tick();
	void tickNet();
	void tickBgm();
	void draw();
	void debugDraw();

	void netAction(Channel * channel, NetAction action, uint8_t param1, uint8_t param2, const std::string & param3 = "");
	void netSyncGameSim(Channel * channel);
	void netAddPlayer(Channel * channel, uint8_t characterIndex, const std::string & displayName, int8_t controllerIndex);
	void netAddPlayerBroadcast(uint16_t owningChannelId, uint8_t index, uint8_t characterIndex, const std::string & displayName, int8_t controllerIndex);
	void netRemovePlayer(Channel * channel, uint8_t index);
	void netRemovePlayerBroadcast(uint8_t index);
	void netSetPlayerInputs(uint16_t channelId, uint8_t playerId, const PlayerInput & input);
	void netSetPlayerInputsBroadcast();
	void netSetPlayerCharacterIndex(uint16_t channelId, uint8_t playerId, uint8_t characterIndex);
	void netBroadcastCharacterIndex(uint8_t playerId, uint8_t characterIndex);
	void netDebugAction(const char * name, const char * param);

	int allocControllerIndex(int preferredControllerIndex);
	void freeControllerIndex(int index);
	int getControllerAllocationCount() const;
	bool isControllerIndexAvailable(int index) const;

	void playSound(const char * filename, int volume = 100);

	std::vector<Client*> getClients() const { return m_clients; }

	static void DialogQuit(void * arg, int dialogId, DialogResult result);

	// Steam integration

	STEAM_CALLBACK_MANUAL(App, OnSteamAvatarImageLoaded, AvatarImageLoaded_t, m_steamAvatarImageLoadedCallback);
	STEAM_CALLBACK_MANUAL(App, OnSteamGameLobbyJoinRequested, GameLobbyJoinRequested_t, m_steamGameLobbyJoinRequestedCallback);
};

extern uint32_t g_buildId;

extern App * g_app;
extern int g_updateTicks;
extern int g_keyboardLock;
extern PlayerInputState * g_uiInput;

extern Surface * g_colorMap;
extern Surface * g_decalMap;
extern Surface * g_lightMap;
extern Surface * g_finalMap;

void inputLockAcquire();
void inputLockRelease();
void applyLightMap(Surface & colormap, Surface & lightmap, Surface & dest);
