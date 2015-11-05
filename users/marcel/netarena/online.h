#pragma once

typedef int OnlineRequestId;
typedef void (*OnlineLobbyFindOrCreateHandler)(OnlineRequestId requestId);
typedef void (*OnlineLobbyLeaveHandler)(OnlineRequestId requestId);

const static OnlineRequestId kOnlineRequestIdInvalid = 0;

class Online
{
public:
	virtual ~Online() { };

	virtual void tick() = 0;

	virtual OnlineRequestId lobbyCreateBegin(OnlineLobbyFindOrCreateHandler * callback) = 0;
	virtual void lobbyCreateEnd(OnlineRequestId id) = 0;

	virtual OnlineRequestId lobbyFindOrCreateBegin(OnlineLobbyFindOrCreateHandler * callback) = 0;
	virtual void lobbyFindOrCreateEnd(OnlineRequestId id) = 0;

	virtual OnlineRequestId lobbyLeaveBegin(OnlineLobbyLeaveHandler * callback) = 0;
	virtual void lobbyLeaveEnd(OnlineRequestId id) = 0;

	virtual void showInviteFriendsUi() = 0;
};

#if 1

#include "steam_api.h"

class OnlineSteam : public Online
{
	enum CallType
	{
		kCallType_None,
		kCallType_LobbyCreate,
		kCallType_LobbyList,
		kCallType_LobbyJoin,
		kCallType_LobbyLeave
	};

	OnlineRequestId m_currentRequestId;

	SteamAPICall_t m_currentCall;
	CallType m_currentCallType;
	bool m_currentCallFailure;
	bool m_currentCallIsDone;

	LobbyCreated_t m_lobbyCreated;
	LobbyMatchList_t m_lobbyMatchList;

	CSteamID m_lobbyId;

	void assertNewCall();

	CCallResult<OnlineSteam, LobbyMatchList_t> m_lobbyMatchListCallback;
	void OnLobbyMatchList(LobbyMatchList_t * lobbyMatchList, bool failure);

	CCallResult<OnlineSteam, LobbyCreated_t> m_lobbyCreatedCallback;
	void OnLobbyCreated(LobbyCreated_t * lobbyCreated, bool failure);

public:
	OnlineSteam();
	virtual ~OnlineSteam();

	virtual void tick();

	virtual OnlineRequestId lobbyCreateBegin(OnlineLobbyFindOrCreateHandler * callback);
	virtual void lobbyCreateEnd(OnlineRequestId id);

	virtual OnlineRequestId lobbyFindOrCreateBegin(OnlineLobbyFindOrCreateHandler * callback);
	virtual void lobbyFindOrCreateEnd(OnlineRequestId id);

	virtual OnlineRequestId lobbyLeaveBegin(OnlineLobbyLeaveHandler * callback);
	virtual void lobbyLeaveEnd(OnlineRequestId id);

	virtual void showInviteFriendsUi();
};

#endif

class OnlineLAN : public Online
{
public:
	OnlineLAN();
	virtual ~OnlineLAN();

	virtual void tick();

	virtual OnlineRequestId lobbyCreateBegin(OnlineLobbyFindOrCreateHandler * callback);
	virtual void lobbyCreateEnd(OnlineRequestId id);

	virtual OnlineRequestId lobbyFindOrCreateBegin(OnlineLobbyFindOrCreateHandler * callback);
	virtual void lobbyFindOrCreateEnd(OnlineRequestId id);

	virtual OnlineRequestId lobbyLeaveBegin(OnlineLobbyLeaveHandler * callback);
	virtual void lobbyLeaveEnd(OnlineRequestId id);

	virtual void showInviteFriendsUi();
};

extern Online * g_online;
