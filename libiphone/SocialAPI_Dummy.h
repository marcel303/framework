#if defined(BBOS)
#include <bps/event.h>
#endif

#include "SocialAPI.h"

#if defined(BBOS)
bool SocialSC_Initialize(const char * gameId, const char * gameVersion, const char * gameKey);
void SocialSC_Shutdown();

bool SocialSCUI_Initialize();
void SocialSCUI_Shutdown();
void SocialSCUI_HandleEvent(bps_event_t * event);
void SocialSCUI_ShowUI();
#endif

class SocialAPI_Dummy : public SocialAPI
{
public:
	SocialAPI_Dummy();
	virtual ~SocialAPI_Dummy();

	virtual bool Initialize(SocialListener * listener);
	virtual void Shutdown();
	virtual void Process();

#if defined(BBOS)
	void HandleEvent(bps_event_t * event);
#endif

	virtual bool IsLoggedIn();
	virtual SocialAsyncHandle AsyncLogin();
	virtual SocialAsyncHandle AsyncLoginNameGet();
	virtual SocialAsyncHandle AsyncScoreSubmit(int mode, int level, int value);
	virtual SocialAsyncHandle AsyncScoreListRange(int mode, int days, int rangeBegin, int rangeCount);
	virtual SocialAsyncHandle AsyncScoreListAroundUser(int mode, int days, int count);
	virtual SocialAsyncHandle AsyncScoreGetUserRank(int mode, int days);
	virtual SocialAsyncHandle AsyncChallengeNew(int userId);
	virtual SocialAsyncHandle AsyncChallengeCancel(int challengeId);
	virtual SocialAsyncHandle AsyncChallengeListPending();
	virtual SocialAsyncHandle AsyncChallengeListCompleted();
	virtual SocialAsyncHandle AsyncFriendAdd(int userId);
	virtual SocialAsyncHandle AsyncFriendList();
	virtual void ScoreAcquire(SocialScore & score);
	virtual void ScoreRelease(SocialScore & score);

//private:
	bool m_isInitialized;
	SocialListener * m_listener;
};
