#pragma once

#include <bps/event.h>
#include <scoreloop/scoreloopcore.h>

class ScoreLoopMgr
{
public:
	ScoreLoopMgr();
	~ScoreLoopMgr();

	bool Initialize(const char * gameId, const char * gameVersion, const char * gameKey);
	void Shutdown();

	void HandleEvent(bps_event_t * event);

	bool IsLoggedIn() const;
	const char * GetUserName();

	void ScoreList(int mode, int days, int begin, int count);
	void ScoreSubmit(int mode, int level, int score);
	void AchievementUnlock();

	void ShowUI();

	CallBack OnScoreSubmitComplete;
	CallBack OnScoreSubmitError;
	CallBack OnScoreListComplete;
	CallBack OnScoreListError;

private:
	static void HandleScoreSubmit(void * obj, SC_Error_t error);
	static void HandleScoreList(void * obj, SC_Error_t error);

	bool m_isInitialized;
	SC_InitData_t m_initData;
	SC_Client_h m_client;
	SCUI_Client_h m_uiClient;
};
