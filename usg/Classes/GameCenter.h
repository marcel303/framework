#include <stdint.h>
#include "CallBack.h"

class GameCenter
{
public:
	GameCenter();
	~GameCenter();
	
	bool IsAvailable();
	bool IsLoggedIn();
	void Login();
	void ScoreSubmit(const char* category, float score);
	void ScoreList(const char* category, uint32_t rangeBegin, uint32_t rangeEnd, uint32_t history);
	
	void ShowGameCenter();
	void ShowLeaderBoard(const char* category);
	
	CallBack OnLoginComplete;
	CallBack OnScoreSubmitComplete;
	CallBack OnScoreSubmitError;
	CallBack OnScoreListComplete;
	CallBack OnScoreListError;
};

extern GameCenter * g_gameCenter;
