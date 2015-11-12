#include "Debugging.h"
#include "gamedefs.h"
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
		m_lobbyEntered = *lobbyEntered;
	m_currentCallFailure = failure;
	m_currentCallIsDone = true;
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
}

OnlineSteam::~OnlineSteam()
{
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

	setFont("calibri.ttf");

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

OnlineRequestId OnlineSteam::lobbyLeaveBegin()
{
	assertNewCall();
	Assert(m_lobbyId.IsLobby());

	LOG_DBG("OnlineSteam: lobbyLeaveBegin");
	SteamMatchmaking()->LeaveLobby(m_lobbyId);
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

uint64_t OnlineSteam::getLobbyOwnerAddress()
{
	Assert(m_lobbyId.IsLobby());
	return SteamMatchmaking()->GetLobbyOwner(m_lobbyId).ConvertToUint64();
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

OnlineRequestId OnlineLAN::lobbyLeaveBegin()
{
	return kOnlineRequestIdInvalid;
}

void OnlineLAN::lobbyLeaveEnd(OnlineRequestId id)
{
	Assert(false);
}

uint64_t OnlineLAN::getLobbyOwnerAddress()
{
	Assert(false);
	return 0;
}

void OnlineLAN::showInviteFriendsUi()
{
	Assert(false);
}
