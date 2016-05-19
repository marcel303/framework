#pragma once

#include "config.h"
#include <stdint.h>

typedef int OnlineRequestId;
typedef uint64_t OnlineLobbyMemberId;

const static OnlineRequestId kOnlineRequestIdInvalid = 0;

class OnlineCallbacks
{
public:
	virtual void OnOnlineLobbyCreateResult(OnlineRequestId requestId, bool success) = 0;
	virtual void OnOnlineLobbyJoinResult(OnlineRequestId requestId, bool success) = 0;
	virtual void OnOnlineLobbyLeaveResult(OnlineRequestId requestId, bool success) = 0;

	virtual void OnOnlineLobbyMemberJoined(OnlineLobbyMemberId memberId) = 0;
	virtual void OnOnlineLobbyMemberLeft(OnlineLobbyMemberId memberId) = 0;
};

class Online
{
public:
	Online(OnlineCallbacks * callbacks) { }
	virtual ~Online() { };

	virtual void tick() = 0;
	virtual void debugDraw() = 0;

	virtual OnlineRequestId lobbyCreateBegin() = 0;
	virtual void lobbyCreateEnd(OnlineRequestId id) = 0;

	virtual OnlineRequestId lobbyFindBegin() = 0;
	virtual void lobbyFindEnd(OnlineRequestId id) = 0;

	virtual OnlineRequestId lobbyJoinBegin(uint64_t gameId) = 0;
	virtual void lobbyJoinEnd(OnlineRequestId id) = 0;

	virtual OnlineRequestId lobbyLeaveBegin() = 0;
	virtual void lobbyLeaveEnd(OnlineRequestId id) = 0;

	virtual bool getLobbyOwnerAddress(uint64_t & lobbyOwnerAddress) = 0;

	virtual void showInviteFriendsUi() = 0;
};

#if ENABLE_STEAM

#include "NetSocket.h"
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

	OnlineCallbacks * m_callbacks;

	OnlineRequestId m_currentRequestId;

	SteamAPICall_t m_currentCall;
	CallType m_currentCallType;
	bool m_currentCallFailure;
	bool m_currentCallIsDone;

	LobbyCreated_t m_lobbyCreated;
	LobbyMatchList_t m_lobbyMatchList;
	LobbyEnter_t m_lobbyEntered;

	CSteamID m_lobbyId;

	void assertNewCall();
	void finalizeCall();

	void lobbyJoinBegin(CSteamID lobbyId);

	CCallResult<OnlineSteam, LobbyMatchList_t> m_lobbyMatchListCallback;
	void OnLobbyMatchList(LobbyMatchList_t * lobbyMatchList, bool failure);

	CCallResult<OnlineSteam, LobbyCreated_t> m_lobbyCreatedCallback;
	void OnLobbyCreated(LobbyCreated_t * lobbyCreated, bool failure);

	CCallResult<OnlineSteam, LobbyEnter_t> m_lobbyJoinedCallback;
	void OnLobbyJoined(LobbyEnter_t * lobbyEntered, bool failure);

	CCallbackManual<OnlineSteam, LobbyChatUpdate_t> m_lobbyChatUpdateCallback;
	void OnLobbyChatUpdate(LobbyChatUpdate_t * lobbyChatUpdate);

	CCallbackManual<OnlineSteam, LobbyKicked_t> m_lobbyKickedCallback;
	void OnLobbyKicked(LobbyKicked_t * lobbyKicked);

public:
	OnlineSteam(OnlineCallbacks * callbacks);
	virtual ~OnlineSteam() override;

	virtual void tick() override;
	virtual void debugDraw() override;

	virtual OnlineRequestId lobbyCreateBegin() override;
	virtual void lobbyCreateEnd(OnlineRequestId id) override;

	virtual OnlineRequestId lobbyFindBegin() override;
	virtual void lobbyFindEnd(OnlineRequestId id) override;

	virtual OnlineRequestId lobbyJoinBegin(uint64_t gameId) override;
	virtual void lobbyJoinEnd(OnlineRequestId id) override;

	virtual OnlineRequestId lobbyLeaveBegin() override;
	virtual void lobbyLeaveEnd(OnlineRequestId id) override;

	virtual bool getLobbyOwnerAddress(uint64_t & lobbyOwnerAddress) override;

	virtual void showInviteFriendsUi() override;
};

class NetSocketSteam : public NetSocket
{
public:
	NetSocketSteam();
	virtual ~NetSocketSteam() override;

	virtual bool Send(const void * data, uint32_t size, NetAddress * address) override;
	virtual bool Receive(void * out_data, uint32_t maxSize, uint32_t * out_size, NetAddress * out_address) override;
	virtual bool IsReliable() override { return true; }
};


#endif

//

class OnlineLocal : public Online
{
	enum CallType
	{
		kCallType_None,
		kCallType_LobbyCreate,
		kCallType_LobbyList,
		kCallType_LobbyJoin,
		kCallType_LobbyLeave
	};

	OnlineCallbacks * m_callbacks;
	OnlineRequestId m_currentRequestId;
	CallType m_currentCallType;

public:
	OnlineLocal(OnlineCallbacks * callbacks);
	virtual ~OnlineLocal() override;

	virtual void tick() override;
	virtual void debugDraw() override;

	virtual OnlineRequestId lobbyCreateBegin() override;
	virtual void lobbyCreateEnd(OnlineRequestId id) override;

	virtual OnlineRequestId lobbyFindBegin() override;
	virtual void lobbyFindEnd(OnlineRequestId id) override;

	virtual OnlineRequestId lobbyJoinBegin(uint64_t gameId) override;
	virtual void lobbyJoinEnd(OnlineRequestId id) override;

	virtual OnlineRequestId lobbyLeaveBegin() override;
	virtual void lobbyLeaveEnd(OnlineRequestId id) override;

	virtual bool getLobbyOwnerAddress(uint64_t & lobbyOwnerAddress) override;

	virtual void showInviteFriendsUi() override;
};

#include "NetSocket.h"

class NetSocketLocal : public NetSocket
{
	struct Packet
	{
		~Packet()
		{
			delete [] data;
			data = nullptr;

			size = 0;

			next = nullptr;
		}

		uint8_t * data;
		uint32_t size;

		Packet * next;
	};

	Packet * m_firstPacket;

public:
	NetSocketLocal();
	virtual ~NetSocketLocal() override;

	virtual bool Send(const void * data, uint32_t size, NetAddress * address) override;
	virtual bool Receive(void * out_data, uint32_t maxSize, uint32_t * out_size, NetAddress * out_address) override;
	virtual bool IsReliable() override { return true; }
};

//

class OnlineLAN : public Online
{
public:
	OnlineLAN(OnlineCallbacks * callbacks);
	virtual ~OnlineLAN() override;

	virtual void tick() override;
	virtual void debugDraw() override;

	virtual OnlineRequestId lobbyCreateBegin() override;
	virtual void lobbyCreateEnd(OnlineRequestId id) override;

	virtual OnlineRequestId lobbyFindBegin() override;
	virtual void lobbyFindEnd(OnlineRequestId id) override;

	virtual OnlineRequestId lobbyJoinBegin(uint64_t gameId) override;
	virtual void lobbyJoinEnd(OnlineRequestId id) override;

	virtual OnlineRequestId lobbyLeaveBegin() override;
	virtual void lobbyLeaveEnd(OnlineRequestId id) override;

	virtual bool getLobbyOwnerAddress(uint64_t & lobbyOwnerAddress) override;

	virtual void showInviteFriendsUi() override;
};

extern Online * g_online;
