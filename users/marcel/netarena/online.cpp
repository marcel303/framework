#include "Debugging.h"
#include "gamedefs.h"
#include "Log.h"
#include "online.h"

Online * g_online = 0;

#if 1

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
}

void OnlineSteam::OnLobbyMatchList(LobbyMatchList_t * lobbyMatchList, bool failure)
{
	if (!failure)
		m_lobbyMatchList = *lobbyMatchList;
	m_currentCallFailure = failure;
	m_currentCallIsDone = true;
}

void OnlineSteam::OnLobbyCreated(LobbyCreated_t * lobbyCreated, bool failure)
{
	if (!failure)
		m_lobbyCreated = *lobbyCreated;
	m_currentCallFailure = failure;
	m_currentCallIsDone = true;
}

//

OnlineSteam::OnlineSteam()
	: Online()
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

		m_currentCallType = kCallType_None;
		m_currentCall = k_uAPICallInvalid;
		m_currentCallFailure = false;
		m_currentCallIsDone = false;

		switch (callType)
		{
		case kCallType_None:
			Assert(false);
			break;

		case kCallType_LobbyCreate:
			if (failure)
			{
				Assert(!m_lobbyId.IsLobby());
				// todo : invoke callback
			}
			else
			{
				m_lobbyId = m_lobbyCreated.m_ulSteamIDLobby;
				Assert(m_lobbyId.IsLobby());

				Verify(SteamMatchmaking()->SetLobbyData(m_lobbyId, "GameMode", "FreeForAll"));

				SteamMatchmaking()->SetLobbyGameServer(m_lobbyId, 0, 0, SteamGameServer()->GetSteamID());
				// todo : invoke callback
			}
			break;

		case kCallType_LobbyList:
			if (failure)
			{
				// todo : invoke callback
			}
			else
			{
				if (m_lobbyMatchList.m_nLobbiesMatching == 0)
				{
					lobbyCreateBegin(0);
				}
				else
				{
					for (int i = 0; i < m_lobbyMatchList.m_nLobbiesMatching; ++i)
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
				}
			}
			break;

		case kCallType_LobbyJoin:
			if (failure)
			{
				// todo : invoke callback
			}
			else
			{
				// todo : invoke callback
			}
			break;
		}
	}
}

OnlineRequestId OnlineSteam::lobbyCreateBegin(OnlineLobbyFindOrCreateHandler * callback)
{
	assertNewCall();

	/*
	k_ELobbyTypePrivate = 0,		// only way to join the lobby is to invite to someone else
	k_ELobbyTypeFriendsOnly = 1,	// shows for friends or invitees, but not in lobby list
	k_ELobbyTypePublic = 2,			// visible for friends and in lobby list
	k_ELobbyTypeInvisible = 3,		// returned by search, but not visible to other friends 
	*/

	m_currentCall = SteamMatchmaking()->CreateLobby(k_ELobbyTypePublic, MAX_PLAYERS);

	if (m_currentCall == k_uAPICallInvalid)
	{
		return kOnlineRequestIdInvalid;
	}
	else
	{
		m_currentRequestId++;
		m_currentCallType = kCallType_LobbyCreate;

		m_lobbyCreatedCallback.Set(m_currentCall, this, &OnlineSteam::OnLobbyCreated);

		return m_currentRequestId;
	}
}

void OnlineSteam::lobbyCreateEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	Assert(m_currentCallType == kCallType_LobbyCreate);
	Assert(m_currentCall != k_uAPICallInvalid);
}

OnlineRequestId OnlineSteam::lobbyFindOrCreateBegin(OnlineLobbyFindOrCreateHandler * callback)
{
	assertNewCall();

	m_currentCall = SteamMatchmaking()->RequestLobbyList();

	if (m_currentCall == k_uAPICallInvalid)
	{
		return kOnlineRequestIdInvalid;
	}
	else
	{
		m_currentRequestId++;
		m_currentCallType = kCallType_LobbyList;

		m_lobbyMatchListCallback.Set(m_currentCall, this, &OnlineSteam::OnLobbyMatchList);

		return m_currentRequestId;
	}
}

void OnlineSteam::lobbyFindOrCreateEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	Assert(m_currentCallType == kCallType_LobbyList || m_currentCallType == kCallType_LobbyCreate);
	Assert(m_currentCall != k_uAPICallInvalid);

	//void AddRequestLobbyListStringFilter( const char *pchKeyToMatch, const char *pchValueToMatch, ELobbyComparison eComparisonType )
	//void AddRequestLobbyListNumericalFilter( const char *pchKeyToMatch, int nValueToMatch, ELobbyComparison eComparisonType )
	//void AddRequestLobbyListNearValueFilter( const char *pchKeyToMatch, int nValueToBeCloseTo )
	//void AddRequestLobbyListFilterSlotsAvailable( int nSlotsAvailable )

	//RequestLobbyList
}

OnlineRequestId OnlineSteam::lobbyLeaveBegin(OnlineLobbyLeaveHandler * callback)
{
	assertNewCall();

	Assert(m_lobbyId.IsValid());

	SteamMatchmaking()->LeaveLobby(m_lobbyId);

	//LeaveLobby( CSteamID steamIDLobby )

	return kOnlineRequestIdInvalid;
}

void OnlineSteam::lobbyLeaveEnd(OnlineRequestId id)
{
	Assert(id == m_currentRequestId);
	Assert(m_currentCallType == kCallType_LobbyLeave);
	Assert(m_currentCall != k_uAPICallInvalid);
	Assert(m_lobbyId.IsValid());

	// todo : invalidate lobby id
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

#endif
