#include "Debugging.h"
#include "gamedefs.h"
#include "libnet_config.h"
#include "Log.h"
#include "online.h"

Online * g_online = 0;

#if 1

#include "framework.h"
#include "steam_gameserver.h"

/*

find friend lobbies:

int cFriends = SteamFriends()->GetFriendCount( k_EFriendFlagImmediate );
for ( int i = 0; i < cFriends; i++ ) 
{
	FriendGameInfo_t friendGameInfo;
	CSteamID steamIDFriend = SteamFriends()->GetFriendByIndex( i, k_EFriendFlagImmediate );
	if ( SteamFriends()->GetFriendGamePlayed( steamIDFriend, &friendGameInfo ) && friendGameInfo.m_steamIDLobby.IsValid() )
	{
		// friendGameInfo.m_steamIDLobby is a valid lobby, you can join it or use RequestLobbyData() get it's metadata
	}
}

invite friends over to lobby:

	bool InviteUserToLobby( CSteamID steamIDLobby, CSteamID steamIDInvitee );

		+connect_lobby <64-bit lobby id> command line or,

		GameLobbyJoinRequested_t callback if in-game

		SteamFriends()->ActivateGameOverlay("LobbyInvite");

avatars:

	virtual int GetMediumFriendAvatar( CSteamID steamIDFriend ) = 0;
	virtual bool RequestUserInformation( CSteamID steamIDUser, bool bRequireNameOnly ) = 0;

receiving lobby callbacks:

	STEAM_CALLBACK(OnlineSteam, OnLobbyDataUpdatedResult, LobbyDataUpdate_t, m_CallbackLobbyDataUpdated);

*/

void OnlineSteam::assertNewCall()
{
	Assert(m_currentCallType == kCallType_None);
	Assert(m_currentCall == k_uAPICallInvalid);
	Assert(m_currentCallFailure == false);
	Assert(m_currentCallIsDone == false);

	memset(&m_lobbyCreated, 0, sizeof(m_lobbyCreated));
	memset(&m_lobbyMatchList, 0, sizeof(m_lobbyMatchList));
	memset(&m_lobbyEntered, 0, sizeof(m_lobbyEntered));
}

void OnlineSteam::finalizeCall()
{
	Assert(m_currentCallType != kCallType_None);
	Assert(m_currentCall != k_uAPICallInvalid);
	Assert(m_currentCallIsDone == true);

	m_currentCallType = kCallType_None;
	m_currentCall = k_uAPICallInvalid;
	m_currentCallFailure = false;
	m_currentCallIsDone = false;
}

void OnlineSteam::lobbyJoinBegin(CSteamID lobbyId)
{
	assertNewCall();

	LOG_DBG("OnlineSteam: lobbyJoinBegin. lobbyId=%llx", lobbyId.ConvertToUint64());
	m_currentCall = SteamMatchmaking()->JoinLobby(lobbyId);
	m_currentCallType = kCallType_LobbyJoin;

	if (m_currentCall == k_uAPICallInvalid)
	{
		LOG_DBG("OnlineSteam: lobbyJoinBegin: failure");
		m_currentCallFailure = true;
		m_currentCallIsDone = true;
	}
	else
	{
		LOG_DBG("OnlineSteam: lobbyJoinBegin: success. requestId=%d", m_currentRequestId);
		m_lobbyJoinedCallback.Set(m_currentCall, this, &OnlineSteam::OnLobbyJoined);
	}
}

//

void OnlineSteam::OnLobbyMatchList(LobbyMatchList_t * lobbyMatchList, bool failure)
{
	LOG_DBG("OnlineSteam: OnLobbyMatchList. failure=%d", failure);
	if (!failure)
		m_lobbyMatchList = *lobbyMatchList;
	m_currentCallFailure = failure;
	m_currentCallIsDone = true;
}

void OnlineSteam::OnLobbyCreated(LobbyCreated_t * lobbyCreated, bool failure)
{
	LOG_DBG("OnlineSteam: OnLobbyCreated. failure=%d", failure);
	if (!failure)
		m_lobbyCreated = *lobbyCreated;
	m_currentCallFailure = failure;
	m_currentCallIsDone = true;
}

void OnlineSteam::OnLobbyJoined(LobbyEnter_t * lobbyEntered, bool failure)
{
	LOG_DBG("OnlineSteam: OnLobbyJoined. failure=%d", failure);
	if (!failure)
	{
		m_lobbyEntered = *lobbyEntered;
		m_lobbyId = lobbyEntered->m_ulSteamIDLobby;
	}
	m_currentCallFailure = failure;
	m_currentCallIsDone = true;
}

void OnlineSteam::OnLobbyChatUpdate(LobbyChatUpdate_t * lobbyChatUpdate)
{
	LOG_DBG("OnlineSteam: OnLobbyChatUpdate");

	/*
	k_EChatMemberStateChangeEntered			= 0x0001,		// This user has joined or is joining the chat room
	k_EChatMemberStateChangeLeft			= 0x0002,		// This user has left or is leaving the chat room
	k_EChatMemberStateChangeDisconnected	= 0x0004,		// User disconnected without leaving the chat first
	k_EChatMemberStateChangeKicked			= 0x0008,		// User kicked
	k_EChatMemberStateChangeBanned			= 0x0010,		// User kicked and banned

	struct LobbyChatUpdate_t
	{
		enum { k_iCallback = k_iSteamMatchmakingCallbacks + 6 };

		uint64 m_ulSteamIDLobby;			// Lobby ID
		uint64 m_ulSteamIDUserChanged;		// user who's status in the lobby just changed - can be recipient
		uint64 m_ulSteamIDMakingChange;		// Chat member who made the change (different from SteamIDUserChange if kicking, muting, etc.)
											// for example, if one user kicks another from the lobby, this will be set to the id of the user who initiated the kick
		uint32 m_rgfChatMemberStateChange;	// bitfield of EChatMemberStateChange values
	};
	*/

	Assert(lobbyChatUpdate->m_ulSteamIDLobby == m_lobbyId.ConvertToUint64());

	if (lobbyChatUpdate->m_ulSteamIDLobby == m_lobbyId.ConvertToUint64())
	{
		if (lobbyChatUpdate->m_rgfChatMemberStateChange & k_EChatMemberStateChangeEntered)
		{
			m_callbacks->OnOnlineLobbyMemberJoined(lobbyChatUpdate->m_ulSteamIDUserChanged);
		}

		if (lobbyChatUpdate->m_rgfChatMemberStateChange & k_EChatMemberStateChangeLeft)
		{
			m_callbacks->OnOnlineLobbyMemberLeft(lobbyChatUpdate->m_ulSteamIDUserChanged);
		}
	}
}

void OnlineSteam::OnLobbyKicked(LobbyKicked_t * lobbyKicked)
{
	LOG_DBG("OnlineSteam: OnLobbyKicked");

	/*
	struct LobbyKicked_t
	{
		enum { k_iCallback = k_iSteamMatchmakingCallbacks + 12 };
		uint64 m_ulSteamIDLobby;			// Lobby
		uint64 m_ulSteamIDAdmin;			// User who kicked you - possibly the ID of the lobby itself
		uint8 m_bKickedDueToDisconnect;		// true if you were kicked from the lobby due to the user losing connection to Steam (currently always true)
	};
	*/

	Assert(lobbyKicked->m_ulSteamIDLobby == m_lobbyId.ConvertToUint64());

	if (lobbyKicked->m_ulSteamIDLobby == m_lobbyId.ConvertToUint64())
	{
		m_callbacks->OnOnlineLobbyMemberLeft(SteamUser()->GetSteamID().ConvertToUint64());
	}
}

//

OnlineSteam::OnlineSteam(OnlineCallbacks * callbacks)
	: Online(callbacks)
	, m_callbacks(callbacks)
	, m_currentRequestId(kOnlineRequestIdInvalid)
	, m_currentCallType(kCallType_None)
	, m_currentCall(k_uAPICallInvalid)
	, m_currentCallFailure(false)
	, m_currentCallIsDone(false)
{
	m_lobbyChatUpdateCallback.Register(this, &OnlineSteam::OnLobbyChatUpdate);
	m_lobbyKickedCallback.Register(this, &OnlineSteam::OnLobbyKicked);
}

OnlineSteam::~OnlineSteam()
{
	m_lobbyChatUpdateCallback.Unregister();
	m_lobbyKickedCallback.Unregister();

	assertNewCall();
}

void OnlineSteam::tick()
{
	if (m_currentCallIsDone)
	{
		Assert(m_currentCallType != kCallType_None);

		const CallType callType = m_currentCallType;
		const SteamAPICall_t call = m_currentCall;
		const bool failure = m_currentCallFailure;

		LOG_DBG("OnlineSteam: current API call is done. callType=%d, call=%llx, failure=%d", callType, call, failure);

		switch (callType)
		{
		case kCallType_None:
			Assert(false);
			break;

		case kCallType_LobbyCreate:
			if (failure)
			{
				LOG_DBG("OnlineSteam: LobbyCreate failure");
				m_callbacks->OnOnlineLobbyCreateResult(m_currentRequestId, false);
			}
			else
			{
				m_lobbyId = m_lobbyCreated.m_ulSteamIDLobby;
				Assert(m_lobbyId.IsLobby());

				LOG_DBG("OnlineSteam: LobbyCreate success. lobbyId=%llx", m_lobbyId.ConvertToUint64());

				Verify(SteamMatchmaking()->SetLobbyData(m_lobbyId, "GameName", "Riposte"));
				Verify(SteamMatchmaking()->SetLobbyData(m_lobbyId, "GameMode", "FreeForAll"));

				SteamMatchmaking()->SetLobbyGameServer(m_lobbyId, 0, 0, SteamGameServer()->GetSteamID());

				m_callbacks->OnOnlineLobbyCreateResult(m_currentRequestId, true);
			}
			break;

		case kCallType_LobbyList:
			if (failure)
			{
				LOG_DBG("OnlineSteam: LobbyList failure");

				m_callbacks->OnOnlineLobbyJoinResult(m_currentRequestId, false);
			}
			else
			{
				LOG_DBG("OnlineSteam: LobbyList success. numLobbies=%d", m_lobbyMatchList.m_nLobbiesMatching);

				if (m_lobbyMatchList.m_nLobbiesMatching == 0)
				{
					m_callbacks->OnOnlineLobbyJoinResult(m_currentRequestId, false);
				}
				else
				{
					for (uint32 i = 0; i < m_lobbyMatchList.m_nLobbiesMatching; ++i)
					{
						const CSteamID lobbyId = SteamMatchmaking()->GetLobbyByIndex(i);
						const int numMembers = SteamMatchmaking()->GetNumLobbyMembers(lobbyId);
						LOG_DBG("num lobby members: %d", numMembers);
						for (int i = 0; i < SteamMatchmaking()->GetLobbyDataCount(lobbyId); ++i)
						{
							char key[1024];
							char value[1024];
							if (SteamMatchmaking()->GetLobbyDataByIndex(lobbyId, i, key, sizeof(key), value, sizeof(value)))
							{
								LOG_DBG("lobby data: %s = %s", key, value);
							}
						}
					}

					finalizeCall();

					//

					const CSteamID lobbyId = SteamMatchmaking()->GetLobbyByIndex(0);

					lobbyJoinBegin(lobbyId);
				}
			}
			break;

		case kCallType_LobbyJoin:
			if (failure)
			{
				LOG_DBG("OnlineSteam: LobbyJoin failure");
				m_callbacks->OnOnlineLobbyJoinResult(m_currentRequestId, false);
			}
			else
			{
				Assert(m_lobbyId.IsLobby());
				LOG_DBG("OnlineSteam: LobbyJoin success");
				m_callbacks->OnOnlineLobbyJoinResult(m_currentRequestId, true);

				/*
				const int numMembers = SteamMatchmaking()->GetNumLobbyMembers(m_lobbyId);
				LOG_DBG("num lobby members: %d", numMembers);
				for (int i = 0; i < numMembers; ++i)
				{
					const CSteamID memberId = SteamMatchmaking()->GetLobbyMemberByIndex(m_lobbyId, i);
					if (memberId.IsValid())
					{
						const int data = 0;
						const int dataSize = sizeof(data);
						Verify(SteamNetworking()->SendP2PPacket(memberId, &data, dataSize, k_EP2PSendReliable));
					}
				}
				*/
			}
			break;

		case kCallType_LobbyLeave:
			if (failure)
			{
				LOG_DBG("OnlineSteam: LobbyLeave failure");
				m_callbacks->OnOnlineLobbyLeaveResult(m_currentRequestId, false);
			}
			else
			{
				LOG_DBG("OnlineSteam: LobbyLeave success");
				m_callbacks->OnOnlineLobbyLeaveResult(m_currentRequestId, true);
			}
			break;

		default:
			Assert(false);
			break;
		}
	}
}

void OnlineSteam::debugDraw()
{
	const int spacing = 32;
	const int fontSize = 30;
	int x = 50;
	int y = 200;

	setDebugFont();

	const int numFriends = SteamFriends()->GetFriendCount(k_EFriendFlagImmediate);
	drawText(x, y += spacing, fontSize, +1.f, +1.f, "steam.friends.numFriends=%d", numFriends);

	if (m_lobbyId.IsLobby())
	{
		const int numMembers = SteamMatchmaking()->GetNumLobbyMembers(m_lobbyId);
		drawText(x, y += spacing, fontSize, +1.f, +1.f, "steam.lobby.numMembers=%d", numMembers);
		for (int i = 0; i < numMembers; ++i)
		{
			CSteamID memberId = SteamMatchmaking()->GetLobbyMemberByIndex(m_lobbyId, i);

			P2PSessionState_t sessionState;
			const bool hasConnection = SteamNetworking()->GetP2PSessionState(memberId, &sessionState);
			if (!hasConnection)
				memset(&sessionState, 0, sizeof(sessionState));

			drawText(x, y += spacing, fontSize, +1.f, +1.f, "steam.lobby.member[%d]=%llx, con=%d/%d, err=%d, relay=%d, sendqueue=%d/%d, ip=%x, port=%d",
				i, memberId.ConvertToUint64(),
				sessionState.m_bConnectionActive,
				sessionState.m_bConnecting,
				sessionState.m_eP2PSessionError,
				sessionState.m_bUsingRelay,
				sessionState.m_nBytesQueuedForSend,
				sessionState.m_nPacketsQueuedForSend,
				sessionState.m_nRemoteIP,
				sessionState.m_nRemotePort);
		}
	}
}

OnlineRequestId OnlineSteam::lobbyCreateBegin()
{
	assertNewCall();

	/*
	k_ELobbyTypePrivate = 0,		// only way to join the lobby is to invite to someone else
	k_ELobbyTypeFriendsOnly = 1,	// shows for friends or invitees, but not in lobby list
	k_ELobbyTypePublic = 2,			// visible for friends and in lobby list
	k_ELobbyTypeInvisible = 3,		// returned by search, but not visible to other friends 
	*/

	LOG_DBG("OnlineSteam: lobbyCreateBegin");
	m_currentCall = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, MAX_PLAYERS);
	m_currentRequestId++;
	m_currentCallType = kCallType_LobbyCreate;

	if (m_currentCall == k_uAPICallInvalid)
	{
		LOG_DBG("OnlineSteam: lobbyCreateBegin: failure");
		m_currentCallFailure = true;
		m_currentCallIsDone = true;
	}
	else
	{
		LOG_DBG("OnlineSteam: lobbyCreateBegin: success. requestId=%d", m_currentRequestId);
		m_lobbyCreatedCallback.Set(m_currentCall, this, &OnlineSteam::OnLobbyCreated);
	}

	return m_currentRequestId;
}

void OnlineSteam::lobbyCreateEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	Assert(m_currentCallType == kCallType_LobbyCreate);
	Assert(m_currentCall != k_uAPICallInvalid);

	LOG_DBG("OnlineSteam: lobbyCreateEnd");

	finalizeCall();
}

OnlineRequestId OnlineSteam::lobbyFindBegin()
{
	assertNewCall();

	LOG_DBG("OnlineSteam: lobbyFindBegin");

	//void AddRequestLobbyListStringFilter( const char *pchKeyToMatch, const char *pchValueToMatch, ELobbyComparison eComparisonType )
	//void AddRequestLobbyListNumericalFilter( const char *pchKeyToMatch, int nValueToMatch, ELobbyComparison eComparisonType )
	//void AddRequestLobbyListNearValueFilter( const char *pchKeyToMatch, int nValueToBeCloseTo )
	//void AddRequestLobbyListFilterSlotsAvailable( int nSlotsAvailable )

	SteamMatchmaking()->AddRequestLobbyListStringFilter("GameName", "Riposte", k_ELobbyComparisonEqual);

	m_currentCall = SteamMatchmaking()->RequestLobbyList();
	m_currentRequestId++;
	m_currentCallType = kCallType_LobbyList;

	if (m_currentCall == k_uAPICallInvalid)
	{
		LOG_DBG("OnlineSteam: lobbyFindBegin: failure");
		m_currentCallFailure = true;
		m_currentCallIsDone = true;
	}
	else
	{
		LOG_DBG("OnlineSteam: lobbyFindBegin: success. requestId=%d", m_currentRequestId);
		m_lobbyMatchListCallback.Set(m_currentCall, this, &OnlineSteam::OnLobbyMatchList);
	}

	return m_currentRequestId;
}

void OnlineSteam::lobbyFindEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	Assert(m_currentCallType == kCallType_LobbyList || m_currentCallType == kCallType_LobbyJoin);
	Assert(m_currentCall != k_uAPICallInvalid);

	LOG_DBG("OnlineSteam: lobbyFindEnd");

	finalizeCall();
}

OnlineRequestId OnlineSteam::lobbyJoinBegin(uint64_t gameId)
{
	assertNewCall();

	LOG_DBG("OnlineSteam: lobbyJoinBegin: gameId=%llx", gameId);

	m_currentRequestId++;

	lobbyJoinBegin(CSteamID(gameId));

	return m_currentRequestId;
}

void OnlineSteam::lobbyJoinEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	Assert(m_currentCallType == kCallType_LobbyJoin);
	Assert(m_currentCall != k_uAPICallInvalid);

	LOG_DBG("OnlineSteam: lobbyJoinEnd");

	finalizeCall();
}

OnlineRequestId OnlineSteam::lobbyLeaveBegin()
{
	assertNewCall();
	Assert(m_lobbyId.IsLobby());

	LOG_DBG("OnlineSteam: lobbyLeaveBegin");
	SteamMatchmaking()->LeaveLobby(m_lobbyId);
	m_currentCall = k_uAPICallInvalid + 1; // fixme : only here to silence asserts. LeaveLobby isn't actually async!
	m_currentRequestId++;
	m_currentCallType = kCallType_LobbyLeave;

	m_currentCallIsDone = true;
	m_currentCallFailure = false;

	return m_currentRequestId;
}

void OnlineSteam::lobbyLeaveEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	Assert(m_currentCallType == kCallType_LobbyLeave);
	Assert(m_currentCall != k_uAPICallInvalid);
	Assert(m_lobbyId.IsLobby());

	LOG_DBG("OnlineSteam: lobbyLeaveEnd");

	m_lobbyId = CSteamID();

	finalizeCall();
}

bool OnlineSteam::getLobbyOwnerAddress(uint64_t & lobbyOwnerAddress)
{
	if (m_lobbyId.IsLobby())
	{
		const CSteamID ownerId = SteamMatchmaking()->GetLobbyOwner(m_lobbyId);
		if (ownerId.IsValid())
		{
			lobbyOwnerAddress = ownerId.ConvertToUint64();
			return true;
		}
	}
	return false;
}

void OnlineSteam::showInviteFriendsUi()
{
	//"Friends", "Community", "Players", "Settings", "OfficialGameGroup", "Stats", "Achievements"

	//SteamFriends()->ActivateGameOverlay("Friends");
	//SteamFriends()->ActivateGameOverlay("OfficialGameGroup");
	//SteamFriends()->ActivateGameOverlay("Achievements");
	//SteamFriends()->ActivateGameOverlay("LobbyInvite");

	Assert(m_lobbyId.IsLobby());
	if (m_lobbyId.IsLobby())
		SteamFriends()->ActivateGameOverlayInviteDialog(m_lobbyId);
}

//

NetSocketSteam::NetSocketSteam()
	: NetSocket()
{
}

NetSocketSteam::~NetSocketSteam()
{
}

bool NetSocketSteam::Send(const void * data, uint32_t size, NetAddress * address)
{
	Assert(size <= LIBNET_SOCKET_MTU_SIZE);

	const CSteamID remoteId(uint64_t(address->m_userData));

	return SteamNetworking()->SendP2PPacket(remoteId, data, size, k_EP2PSendReliable);
}

bool NetSocketSteam::Receive(void * out_data, uint32_t maxSize, uint32_t * out_size, NetAddress * out_address)
{
	uint32_t packetSize;
	if (SteamNetworking()->IsP2PPacketAvailable(&packetSize))
	{
		if (packetSize > maxSize)
		{
			LOG_ERR("IsP2PPacketAvailable: packetSize (%d) > maxSize (%d)", packetSize, maxSize);
		}
		else
		{
			uint32_t readSize;
			CSteamID fromId;
			if (SteamNetworking()->ReadP2PPacket(out_data, maxSize, &readSize, &fromId))
			{
				Assert(readSize == packetSize);
				if (readSize == packetSize)
				{
					*out_size = readSize;
					out_address->Set(127, 0, 0, 1, 666);
					out_address->m_userData = fromId.ConvertToUint64();
					return true;
				}
			}
		}
	}

	return false;
}

#endif

//

OnlineLocal::OnlineLocal(OnlineCallbacks * callbacks)
	: Online(callbacks)
	, m_callbacks(callbacks)
	, m_currentRequestId(0)
	, m_currentCallType(kCallType_None)
{
}

OnlineLocal::~OnlineLocal()
{
	m_callbacks = nullptr;
}

void OnlineLocal::tick()
{
	if (m_currentCallType == kCallType_None)
		return;

	switch (m_currentCallType)
	{
	case kCallType_LobbyCreate:
		m_callbacks->OnOnlineLobbyCreateResult(m_currentRequestId, true);
		break;

	case kCallType_LobbyList:
		//m_callbacks->OnOnlineLobbyListResult(m_currentRequestId, true);
		break;

	case kCallType_LobbyJoin:
		m_callbacks->OnOnlineLobbyJoinResult(m_currentRequestId, true);
		break;

	case kCallType_LobbyLeave:
		m_callbacks->OnOnlineLobbyLeaveResult(m_currentRequestId, true);
		break;

	default:
		Assert(false);
		break;
	}

	Assert(m_currentCallType == kCallType_None);
	m_currentCallType = kCallType_None;
}

void OnlineLocal::debugDraw()
{
}

OnlineRequestId OnlineLocal::lobbyCreateBegin()
{
	Assert(m_currentCallType == kCallType_None);
	m_currentRequestId++;
	m_currentCallType = kCallType_LobbyCreate;
	return m_currentRequestId;
}

void OnlineLocal::lobbyCreateEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	m_currentCallType = kCallType_None;
}

OnlineRequestId OnlineLocal::lobbyFindBegin()
{
	Assert(m_currentCallType == kCallType_None);
	m_currentRequestId++;
	//m_currentCallType == kCallType_LobbyFind;
	Assert(false);
	return m_currentRequestId;
}

void OnlineLocal::lobbyFindEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	m_currentCallType = kCallType_None;
}

OnlineRequestId OnlineLocal::lobbyJoinBegin(uint64_t gameId)
{
	Assert(m_currentCallType == kCallType_None);
	m_currentRequestId++;
	m_currentCallType = kCallType_LobbyJoin;
	return m_currentRequestId;
}

void OnlineLocal::lobbyJoinEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	m_currentCallType = kCallType_None;
}

OnlineRequestId OnlineLocal::lobbyLeaveBegin()
{
	Assert(m_currentCallType == kCallType_None);
	m_currentRequestId++;
	m_currentCallType = kCallType_LobbyLeave;
	return m_currentRequestId;
}

void OnlineLocal::lobbyLeaveEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	m_currentCallType = kCallType_None;
}

bool OnlineLocal::getLobbyOwnerAddress(uint64_t & lobbyOwnerAddress)
{
	lobbyOwnerAddress = 0;
	return true;
}

void OnlineLocal::showInviteFriendsUi()
{
}

NetSocketLocal::NetSocketLocal()
	: m_firstPacket(nullptr)
{
}

NetSocketLocal::~NetSocketLocal()
{
	while (m_firstPacket)
	{
		Packet * nextPacket = m_firstPacket->next;
		delete m_firstPacket;
		m_firstPacket = nextPacket;
	}
}

bool NetSocketLocal::Send(const void * data, uint32_t size, NetAddress * address)
{
	if (size == 0)
		return true;

	Packet * packet = new Packet;
	packet->data = new uint8_t[size];
	packet->size = size;
	packet->next = nullptr;
	memcpy(packet->data, data, size);

	Packet ** insertAt = &m_firstPacket;

	while (*insertAt)
		insertAt = &(*insertAt)->next;

	*insertAt = packet;

	return true;
}

bool NetSocketLocal::Receive(void * out_data, uint32_t maxSize, uint32_t * out_size, NetAddress * out_address)
{
	if (m_firstPacket)
	{
		Assert(m_firstPacket->size <= maxSize);
		const uint32 copySize = m_firstPacket->size <= maxSize ? m_firstPacket->size : maxSize;

		memcpy(out_data, m_firstPacket->data, copySize);
		*out_size = copySize;

		out_address->Set(127, 0, 0, 1, 666);
		out_address->m_userData = 0;

		Packet * nextPacket = m_firstPacket->next;
		delete m_firstPacket;
		m_firstPacket = nextPacket;

		return true;
	}
	else
	{
		return false;
	}
}

//

OnlineLAN::OnlineLAN(OnlineCallbacks * callbacks)
	: Online(callbacks)
{
}

OnlineLAN::~OnlineLAN()
{
}

void OnlineLAN::tick()
{
}

void OnlineLAN::debugDraw()
{
}

OnlineRequestId OnlineLAN::lobbyCreateBegin()
{
	return kOnlineRequestIdInvalid;
}

void OnlineLAN::lobbyCreateEnd(OnlineRequestId id)
{
	Assert(false);
}

OnlineRequestId OnlineLAN::lobbyFindBegin()
{
	return kOnlineRequestIdInvalid;
}

void OnlineLAN::lobbyFindEnd(OnlineRequestId id)
{
	Assert(false);
}

OnlineRequestId OnlineLAN::lobbyJoinBegin(uint64_t gameId)
{
	return kOnlineRequestIdInvalid;
}

void OnlineLAN::lobbyJoinEnd(OnlineRequestId id)
{
	Assert(false);
}

OnlineRequestId OnlineLAN::lobbyLeaveBegin()
{
	return kOnlineRequestIdInvalid;
}

void OnlineLAN::lobbyLeaveEnd(OnlineRequestId id)
{
	Assert(false);
}

bool OnlineLAN::getLobbyOwnerAddress(uint64_t & lobbyOwnerAddress)
{
	Assert(false);
	return false;
}

void OnlineLAN::showInviteFriendsUi()
{
	Assert(false);
}
