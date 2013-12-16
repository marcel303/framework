#include "Debugging.h"
#include "grs.h"
#include "Log.h"
#include "ScoreLoopMgr.h"

#include <bps/dialog.h> // todo: move

ScoreLoopMgr::ScoreLoopMgr()
	: m_isInitialized(false)
{
}

ScoreLoopMgr::~ScoreLoopMgr()
{
	Assert(m_isInitialized == false);
	if (m_isInitialized)
		Shutdown();
}

bool ScoreLoopMgr::Initialize(const char * gameId, const char * gameVersion, const char * gameKey)
{
	Assert(m_isInitialized == false);

	SC_Error_t error;

	SC_InitData_Init(&m_initData);

	const char * currencyCode = "HVY";
	const char * lang = "en";

	error = SC_Client_New(
		&m_client,
		&m_initData,
	    	gameId,
	    	gameKey,
	    	gameVersion,
	    	currencyCode,
	    	lang);

	if (error != SC_OK)
	{
		LOG_ERR("failed to create scoreloop client", 0);
		return false;
	}

	SC_Session_h session = SC_Client_GetSession(m_client);

	if (session == NULL)
	{
		LOG_INF("scoreloop session is NULL", 0);
	}
	else
	{
		SC_Game_h game = SC_Session_GetGame(session);

		if (game == NULL)
		{
			LOG_INF("scoreloop game is NULL", 0);
		}
		else
		{
			SC_User_h user = SC_Session_GetUser(session);

			if (user == NULL)
			{
				LOG_INF("scoreloop user is NULL", 0);
			}
			else
			{
				bool isFavorite = SC_User_IsFavoriteGame(user, game);

				LOG_INF("scoreloop: is favorite: %d", isFavorite ? 1 : 0);
			}
		}
	}

	error = SCUI_Client_New(&m_uiClient, m_client);

	if (error != SC_OK)
	{
		LOG_ERR("failed to create scoreloop UI client", 0);
		return false;
	}

	m_isInitialized = true;

#if 1
	dialog_instance_t dialog = NULL;
	dialog_create_alert(&dialog);
	dialog_set_title_text(dialog, "Add friend");
	dialog_set_alert_message_text(dialog, "Do you want to add ... as a friend?");
	dialog_add_button(dialog, DIALOG_OK_LABEL, true, NULL, true);
	dialog_show(dialog);
#endif

	return true;
}

void ScoreLoopMgr::Shutdown()
{
	Assert(m_isInitialized);

	SCUI_Client_Release(m_uiClient);

	SC_Client_Release(m_client);

	m_isInitialized = false;
}

void ScoreLoopMgr::HandleEvent(bps_event_t * event)
{
	int domain = bps_event_get_domain(event);

	if (domain == SC_GetBPSEventDomain(&m_initData))
	{
		SC_HandleBPSEvent(&m_initData, event);
	}

	SCUI_Client_HandleEvent(m_uiClient, event);
}

bool ScoreLoopMgr::IsLoggedIn() const
{
	return true;
}

const char * ScoreLoopMgr::GetUserName()
{
	return "local-user";
}

void ScoreLoopMgr::HandleScoreSubmit(void * obj, SC_Error_t error)
{
	ScoreLoopMgr * self = (ScoreLoopMgr*)obj;

	if (error != SC_OK)
	{
		LOG_ERR("score submit failed", 0);

		if (self->OnScoreSubmitError.IsSet())
			self->OnScoreSubmitError.Invoke(0);
	}
	else
	{
		LOG_INF("score submit ok", 0);

		GRS::QueryResult qr;

		qr.m_RankCurr = 0;

		if (self->OnScoreSubmitComplete.IsSet())
			self->OnScoreSubmitComplete.Invoke(&qr);
	}
}

class ScoreListArgs
{
public:
	ScoreLoopMgr * mgr;
	SC_ScoresController_h controller;
	int rangeBegin;
};

void ScoreLoopMgr::HandleScoreList(void * obj, SC_Error_t error)
{
	ScoreListArgs * args = (ScoreListArgs*)obj;
	ScoreLoopMgr * self = args->mgr;

	if (error != SC_OK)
	{
		LOG_ERR("score list failed", 0);

		if (self->OnScoreListError.IsSet())
			self->OnScoreListError.Invoke(0);
	}
	else
	{
		LOG_INF("score list ok", 0);

		SC_ScoresController_h controller = args->controller;

		SC_ScoreList_h scores = SC_ScoresController_GetScores(controller);

		int count = SC_ScoreList_GetCount(scores);

		LOG_DBG("got %d scores", count);

		//construct GRS query result object for compatibility's sake

		GRS::QueryResult qr;

		qr.m_ScoreCount = count;
		qr.m_ResultBegin = args->rangeBegin;

		for (int i = 0; i < count; ++i)
		{
			SC_Score_h score = SC_ScoreList_GetAt(scores, i);

			if (i == 0)
			{
				int rank = SC_Score_GetRank(score);
				qr.m_ResultBegin = rank - 1;
				LOG_INF("first rank: %d", rank);
			}

			int mode = SC_Score_GetMode(score);
			int level = SC_Score_GetLevel(score);
			int value = SC_Score_GetResult(score);
			SC_User_h user = SC_Score_GetUser(score);

			//SC_User_GetNationality

			const char * playerName = SC_String_GetData(SC_User_GetLogin(user));

			//SC_ScoreFormatter_h scoreFormatter = SC_Client_GetScoreFormatter(self->m_client);

			//SC_String_h formattedScore;

			//error = SC_ScoreFormatter_FormatScore(scoreFormatter, score, SC_SCORE_FORMAT_DEFAULT, &formattedScore);
			//Assert(error == SC_OK);

			//const char * playerName = SC_String_GetData(formattedScore);

			//if (playerName == 0)
			//	playerName = "n/a";

			LOG_DBG("got score: mode=%d, level=%d, value=%d", mode, level, value);

			GRS::HighScore gs;

			gs.Setup(0, 0, GRS::UserId(""), playerName, "", static_cast<uint32_t>(value) / 100.0f, GRS::TimeStamp(), GRS::GpsLocation(), GRS::CountryCode(), false);

			qr.m_Scores.push_back(gs);

			//SC_String_Release(formattedScore);
		}

		if (self->OnScoreListComplete.IsSet())
			self->OnScoreListComplete.Invoke(&qr);
	}

	delete args;
}

void ScoreLoopMgr::ScoreList(int mode, int days, int begin, int count)
{
	SC_Error_t error;

	ScoreListArgs * args = new ScoreListArgs();

	SC_ScoresController_h controller;

	error = SC_Client_CreateScoresController(m_client, &controller, HandleScoreList, args);

	SC_TimeInterval_t timeInterval;

	if (days == 0)
		timeInterval = SC_TIME_INTERVAL_ALL;
	else if (days == 7)
		timeInterval = SC_TIME_INTERVAL_7DAYS;
	else if (days == 30)
		timeInterval = SC_TIME_INTERVAL_30DAYS;
	else
	{
		Assert(false);
		timeInterval = SC_TIME_INTERVAL_ALL;
	}

	SC_ScoresSearchList_t searchList;
	searchList.timeInterval = timeInterval;
	searchList.countrySelector = SC_COUNTRY_SELECTOR_ALL;
	searchList.country = NULL;
	searchList.usersSelector = SC_USERS_SELECTOR_ALL; // SC_USERS_SELECTOR_BUDDYHOOD

	SC_ScoresController_SetSearchList(controller, searchList);

	SC_ScoresController_SetMode(controller, mode);

	args->mgr = this;
	args->controller = controller;
	args->rangeBegin = begin;

	SC_Range_t range;
	range.offset = begin;
	range.length = count;

	//error = SC_ScoresController_LoadScores(controller, range);

	SC_Session_h session = SC_Client_GetSession(m_client);
	SC_User_h user = SC_Session_GetUser(session);
	error = SC_ScoresController_LoadScoresAroundUser(controller, user, count * 2);

	if (error != SC_OK)
	{
		LOG_ERR("failed to list scores", 0);
	}
	else
	{
		LOG_INF("queried scores", 0);
	}
}

void ScoreLoopMgr::ScoreSubmit(int mode, int level, int value)
{
	SC_Error_t error;
	
	SC_Score_h score;

	error = SC_Client_CreateScore(m_client, &score);
	Assert(error == SC_OK);

	error = SC_Score_SetMode(score, mode);
	Assert(error == SC_OK);

	error = SC_Score_SetLevel(score, level);
	Assert(error == SC_OK);

	error = SC_Score_SetResult(score, value);
	Assert(error == SC_OK);

	double minorScore = 0.0;
	SC_Score_SetMinorResult(score, minorScore);
	Assert(error == SC_OK);

	SC_ScoreController_h controller;

	error = SC_Client_CreateScoreController(m_client, &controller, HandleScoreSubmit, this);

	if (error != SC_OK)
	{
		LOG_ERR("failed to create score controller", 0);
	}
	else
	{
		error = SC_ScoreController_SubmitScore(controller, score);

		if (error != SC_OK)
		{
			LOG_ERR("failed to submit score", 0);
		}
		else
		{
			LOG_INF("submitted score", 0);
		}
	}
}

static void HandleUIClosed(void * obj, SCUI_Result_t result, const void * data)
{
	LOG_ERR("scoreloop UI result: %d", result);
}

void ScoreLoopMgr::ShowUI()
{
	SC_Error_t error;

	error = SCUI_Client_ShowFavoriteGamesView(m_uiClient, HandleUIClosed, this);

	if (error != SC_OK)
	{
		LOG_ERR("failed to show scoreloop UI: %d", error);
	}
}
