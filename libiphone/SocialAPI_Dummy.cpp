#include "SocialAPI_Dummy.h"

#if defined(BBOS)
bool SocialSC_Initialize(const char * gameId, const char * gameVersion, const char * gameKey)
{
}

void SocialSC_Shutdown()
{
}
#endif

static int m_handleAlloc = 0;
static SocialAsyncHandle CreateHandle()
{
	m_handleAlloc++;
	if (m_handleAlloc <= 0)
		m_handleAlloc = 1;
	SocialAsyncHandle handle;
	handle.m_id = m_handleAlloc;
	return handle;
}

SocialAPI_Dummy::SocialAPI_Dummy()
{
}

SocialAPI_Dummy::~SocialAPI_Dummy()
{
}

bool SocialAPI_Dummy::Initialize(SocialListener * listener)
{
	return true;
}

void SocialAPI_Dummy::Shutdown()
{
}

void SocialAPI_Dummy::Process()
{
}

#if defined(BBOS)
void SocialAPI_Dummy::HandleEvent(bps_event_t * event)
{
}
#endif

bool SocialAPI_Dummy::IsLoggedIn()
{
	return true;
}

SocialAsyncHandle SocialAPI_Dummy::AsyncLogin()
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncLoginNameGet()
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncScoreSubmit(int mode, int level, int value)
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncScoreListRange(int mode, int days, int rangeBegin, int rangeCount)
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncScoreListAroundUser(int mode, int days, int count)
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncScoreGetUserRank(int mode, int days)
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncChallengeNew(int userId)
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncChallengeCancel(int challengeId)
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncChallengeListPending()
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncChallengeListCompleted()
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncFriendAdd(int userId)
{
	return CreateHandle();
}

SocialAsyncHandle SocialAPI_Dummy::AsyncFriendList()
{
	return CreateHandle();
}

void SocialAPI_Dummy::ScoreAcquire(SocialScore & score)
{
}

void SocialAPI_Dummy::ScoreRelease(SocialScore & score)
{
}

#if defined(BBOS)

// SCUI

bool SocialSCUI_Initialize()
{
	return true;
}

void SocialSCUI_Shutdown()
{
}

void SocialSCUI_HandleEvent(bps_event_t * event)
{
}

#endif
