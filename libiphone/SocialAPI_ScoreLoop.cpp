#include <string.h>
#include "Debugging.h"
#include "SocialAPI_ScoreLoop.h"

#include <bps/dialog.h> // todo: move

static void HandleScoreSubmit(void * obj, SC_Error_t error);
static void HandleScoreList(void * obj, SC_Error_t error);

static bool g_isInitialized = false;
static SC_InitData_t g_initData;
static SC_Client_h g_client = NULL;
static SCUI_Client_h g_uiClient = NULL;

bool SocialSC_Initialize(const char * gameId, const char * gameVersion, const char * gameKey)
{
	Assert(g_isInitialized == false);

	LOG_INF("initializing ScoreLoop", 0);

	SC_Error_t error = SC_OK;

	if (error == SC_OK)
	{
		SC_InitData_Init(&g_initData);
	}

	//

	if (error == SC_OK)
	{
		char versionText[256];

		if (SC_GetVersionInfo(&g_initData, versionText, sizeof(versionText)))
		{
			LOG_INF("scoreloop version: %s", versionText);
		}
	}

	if (error == SC_OK)
	{
		const char * currencyCode = "HVY";
		const char * lang = "en";

		error = SC_Client_New(
			&g_client,
			&g_initData,
			gameId,
			gameKey,
			gameVersion,
			currencyCode,
			lang);
	}

	if (error == SC_OK && g_client == NULL)
	{
		error = SC_INVALID_STATE;
	}

	if (error != SC_OK)
	{
		LOG_ERR("failed to create scoreloop client: %d", error);
		SocialSC_Shutdown();
		return false;
	}

	SC_Session_h session = SC_Client_GetSession(g_client);

	if (session == NULL)
	{
		LOG_ERR("scoreloop session is NULL", 0);
	}
	else
	{
		SC_Game_h game = SC_Session_GetGame(session);

		if (game == NULL)
		{
			LOG_ERR("scoreloop game is NULL", 0);
		}
		else
		{
			SC_User_h user = SC_Session_GetUser(session);

			if (user == NULL)
			{
				LOG_ERR("scoreloop user is NULL", 0);
			}
			else
			{
				bool isFavorite = SC_User_IsFavoriteGame(user, game);

				LOG_INF("scoreloop: is favorite: %d", isFavorite ? 1 : 0);
			}
		}
	}

	LOG_INF("initializing ScoreLoop [done]", 0);

	g_isInitialized = true;

#if 0
	dialog_instance_t dialog = NULL;
	dialog_create_alert(&dialog);
	dialog_set_title_text(dialog, "Add friend");
	dialog_set_alert_message_text(dialog, "Do you want to add ... as a friend?");
	dialog_add_button(dialog, DIALOG_OK_LABEL, true, NULL, true);
	dialog_show(dialog);
#endif

	return true;
}

void SocialSC_Shutdown()
{
	Assert(g_isInitialized);

	LOG_INF("shutting down ScoreLoop", 0);

	if (g_client != NULL)
	{
		SC_Client_Release(g_client);
		g_client = NULL;
	}

	LOG_INF("shutting down ScoreLoop [done]", 0);

	g_isInitialized = false;
}

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

SocialSC::SocialSC()
	: m_isInitialized(false)
	, m_listener(NULL)
{
}

SocialSC::~SocialSC()
{
	Assert(m_isInitialized == false);
	if (m_isInitialized)
		Shutdown();
}

bool SocialSC::Initialize(SocialListener * listener)
{
	Assert(g_isInitialized);
	Assert(m_isInitialized == false);
	if (!g_isInitialized)
		return false;

	m_listener = listener;

	m_isInitialized = true;

	return true;
}

void SocialSC::Shutdown()
{
	Assert(g_isInitialized);
	Assert(m_isInitialized);

	m_listener = NULL;

	m_isInitialized = false;
}

void SocialSC::Process()
{
}

void SocialSC::HandleEvent(bps_event_t * event)
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return;

	int domain = bps_event_get_domain(event);

	if (domain == SC_GetBPSEventDomain(&g_initData))
	{
		SC_HandleBPSEvent(&g_initData, event);
	}
}

bool SocialSC::IsLoggedIn()
{
	Assert(m_isInitialized);
	return true;
}

SocialAsyncHandle SocialSC::AsyncLogin()
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	// todo: schedule callback

	return CreateHandle();
}

SocialAsyncHandle SocialSC::AsyncLoginNameGet()
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	// todo: return name

	return CreateHandle();
}

//

static SC_ScoresSearchList_t MakeSearchList(int days)
{
	// set time interval

	SC_TimeInterval_t timeInterval;
	memset(&timeInterval, 0, sizeof(SC_TimeInterval_t));

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

	return searchList;
}

//

class ScoreSubmitArgs
{
public:
	enum State
	{
		State_Submit,
		State_GetRank
	};

	ScoreSubmitArgs()
		: self(NULL)
		, controller(NULL)
		, score(NULL)
	{
	}

	SocialSC * self;
	SocialAsyncHandle handle;
	SC_ScoreController_h controller;
	SC_Score_h score;
};

static void HandleScoreSubmit(void * obj, SC_Error_t error)
{
	ScoreSubmitArgs * args = (ScoreSubmitArgs*)obj;
	SocialSC * self = args->self;

	if (error != SC_OK)
	{
		LOG_ERR("score submit failed: %d", error);

		self->m_listener->HandleScoreSubmit(args->handle, kSocialError, -1);
	}
	else
	{
		LOG_INF("score submit succeeded", 0);

		//SC_Score_h score = SC_ScoreController_GetScore(args->controller);
		//int rank = score ? SC_Score_GetRank(score) - 1 : -1;

		int rank = -1;

		self->m_listener->HandleScoreSubmit(args->handle, kSocialOK, rank);
	}

	Assert(args->controller != NULL);
	SC_ScoreController_Release(args->controller);
	args->controller = NULL;

	Assert(args->score != NULL);
	SC_Score_Release(args->score);
	args->score = NULL;

	delete args;
	args = NULL;
}

SocialAsyncHandle SocialSC::AsyncScoreSubmit(int mode, int level, int value)
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	SC_Error_t error = SC_OK;

	SocialAsyncHandle handle = CreateHandle();

	ScoreSubmitArgs * args = NULL;
	SC_Score_h score = NULL;
	SC_ScoreController_h controller = NULL;

	args = new ScoreSubmitArgs();

	if (error == SC_OK)
	{
		error = SC_Client_CreateScore(g_client, &score);
		Assert(error == SC_OK);

		if (error != SC_OK)
		{
			LOG_ERR("failed to create score: %d", error);
		}
	}

	if (error == SC_OK)
	{
		error = SC_Score_SetMode(score, mode);
		Assert(error == SC_OK);
	}

	if (error == SC_OK)
	{
		error = SC_Score_SetLevel(score, level);
		Assert(error == SC_OK);
	}

	if (error == SC_OK)
	{
		error = SC_Score_SetResult(score, value);
		Assert(error == SC_OK);
	}

	if (error == SC_OK)
	{
		double minorScore = 0.0;
		SC_Score_SetMinorResult(score, minorScore);
		Assert(error == SC_OK);
	}

	if (error == SC_OK)
	{
		error = SC_Client_CreateScoreController(g_client, &controller, HandleScoreSubmit, args);

		if (error != SC_OK)
		{
			LOG_ERR("failed to create score controller: %d", error);
		}
	}

	if (error == SC_OK)
	{
		args->self = this;
		args->handle = handle;
		args->controller = controller;
		args->score = score;
	}

	if (error == SC_OK)
	{
		error = SC_ScoreController_SubmitScore(controller, score);

		if (error != SC_OK)
		{
			LOG_ERR("failed to submit score", 0);
		}
	}

	if (error == SC_OK)
	{
		LOG_INF("submitted score", 0);
	}
	else
	{
		if (controller != NULL)
		{
			SC_ScoreController_Release(controller);
			controller = NULL;
		}


		if (score != NULL)
		{
			SC_Score_Release(score);
			score = NULL;
		}

		if (args != NULL)
		{
			delete args;
			args = NULL;
		}

		// todo: schedule error callback
	}

	return handle;
}

//

class ScoreListArgs
{
public:
	ScoreListArgs()
		: self(NULL)
		, controller(NULL)
		, rangeBegin(0)
		, rangeCount(0)
		, aroundUser(false)
	{
	}

	SocialSC * self;
	SocialAsyncHandle handle;
	SC_ScoresController_h controller;
	int rangeBegin;
	int rangeCount;
	bool aroundUser;
};

static void HandleScoreList(void * obj, SC_Error_t error)
{
	ScoreListArgs * args = (ScoreListArgs*)obj;
	SocialSC * self = args->self;

	if (error != SC_OK)
	{
		LOG_ERR("score list failed: %d", error);

		if (args->aroundUser)
		{
			SocialScoreList scores;
			self->m_listener->HandleScoreListAroundUser(args->handle, kSocialError, scores);
		}
		else
		{
			SocialScoreList scores;
			self->m_listener->HandleScoreListRange(args->handle, kSocialError, scores);
		}
	}
	else
	{
		LOG_INF("score list ok", 0);

		SC_ScoresController_h controller = args->controller;
		SC_ScoreList_h scores = SC_ScoresController_GetScores(controller);
		int count = scores ? SC_ScoreList_GetCount(scores) : 0;

		LOG_DBG("got %d scores", count);

		// construct score list

		SocialScoreList socialScores;

		for (int i = 0; i < count; ++i)
		{
			SC_Score_h score = SC_ScoreList_GetAt(scores, i);
			Assert(score != NULL);

			const int mode = SC_Score_GetMode(score);
			const int level = SC_Score_GetLevel(score);
			const int value = SC_Score_GetResult(score);
			const int rank = SC_Score_GetRank(score) - 1;
			SC_User_h user = SC_Score_GetUser(score);
			const char * userName = SC_String_GetData(SC_User_GetLogin(user));

			LOG_DBG("got score: mode=%d, level=%d, value=%d", mode, level, value);

			SocialScore socialScore(mode, level, value, rank, userName);
			socialScore.m_user = user;

			socialScores.scores.push_back(socialScore);
		}

		if (args->aroundUser)
		{
			socialScores.scoreCount = -1;

			self->m_listener->HandleScoreListAroundUser(args->handle, kSocialOK, socialScores);
		}
		else
		{
			if (count < args->rangeCount)
			{
				// ScoreLoop returned less results than expected. we must have hit the end of the list
				socialScores.scoreCount = args->rangeBegin + count;
			}
			else
			{
				// we haven't hit the bottom yet. allow for more scores to be queried
				socialScores.scoreCount = -1;
			}

			self->m_listener->HandleScoreListRange(args->handle, kSocialOK, socialScores);
		}
	}

	SC_ScoresController_Release(args->controller);
	args->controller = NULL;

	delete args;
	args = NULL;
}

static void ScoreList(SocialSC * self, SC_Client_h client, SocialAsyncHandle handle, int mode, int days, int rangeBegin, int rangeCount, int aroundUserCount, bool aroundUser)
{
	SC_Error_t error = SC_OK;

	ScoreListArgs * args = NULL;
	SC_ScoresController_h controller = NULL;

	if (error == SC_OK)
	{
		args = new ScoreListArgs();
	}

	// create controller

	if (error == SC_OK)
	{
		error = SC_Client_CreateScoresController(client, &controller, HandleScoreList, args);

		if (error != SC_OK)
		{
			LOG_ERR("failed to create scores controller: %d", error);
		}
	}

	// set search criteria

	if (error == SC_OK)
	{
		SC_ScoresSearchList_t searchList = MakeSearchList(days);
		SC_ScoresController_SetSearchList(controller, searchList);
	}

	// set game mode

	if (error == SC_OK)
	{
		SC_ScoresController_SetMode(controller, mode);
	}

	//

	if (error == SC_OK)
	{
		args->self = self;
		args->handle = handle;
		args->controller = controller;
		args->rangeBegin = rangeBegin;
		args->rangeCount = rangeCount;
		args->aroundUser = aroundUser;
	}

	// list scores

	if (aroundUser)
	{
		SC_Session_h session = NULL;
		SC_User_h user = NULL;

		if (error == SC_OK)
		{
			session = SC_Client_GetSession(client);
			Assert(session != NULL);
			if (session == NULL)
			{
				error = SC_INVALID_STATE;
			}
		}

		if (error == SC_OK)
		{
			user = SC_Session_GetUser(session);
			Assert(user != NULL);
			if (user == NULL)
			{
				error = SC_INVALID_STATE;
			}
		}

		if (error == SC_OK)
		{
			error = SC_ScoresController_LoadScoresAroundUser(controller, user, aroundUserCount);

			if (error != SC_OK)
			{
				LOG_ERR("failed to load scores around user: %d", error);
			}
		}
	}
	else
	{
		if (error == SC_OK)
		{
			SC_Range_t range;
			range.offset = rangeBegin;
			range.length = rangeCount;

			error = SC_ScoresController_LoadScores(controller, range);

			if (error != SC_OK)
			{
				LOG_ERR("failed to load scores: %d", error);
			}
		}
	}

	if (error == SC_OK)
	{
		LOG_INF("queried scores", 0);
	}
	else
	{
		LOG_ERR("failed to list scores", 0);

		if (controller != NULL)
		{
			SC_ScoresController_Release(controller);
			controller = NULL;
		}

		if (args != NULL)
		{
			delete args;
			args = NULL;
		}

		// todo: schedule error callback
	}
}

SocialAsyncHandle SocialSC::AsyncScoreListRange(int mode, int days, int rangeBegin, int rangeCount)
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	SocialAsyncHandle handle = CreateHandle();

	ScoreList(this, g_client, handle, mode, days, rangeBegin, rangeCount, 0, false);

	return handle;
}

SocialAsyncHandle SocialSC::AsyncScoreListAroundUser(int mode, int days, int count)
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	SocialAsyncHandle handle = CreateHandle();

	ScoreList(this, g_client, handle, mode, days, 0, 0, count, true);

	return handle;
}

class ScoreGetUserRankArgs
{
public:
	ScoreGetUserRankArgs()
		: self(NULL)
	{
	}

	SocialSC * self;
	SocialAsyncHandle handle;
};

SocialAsyncHandle SocialSC::AsyncScoreGetUserRank(int mode, int days)
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	return CreateHandle(); // todo
/*
	ScoreGetUserRankArgs * args = new ScoreGetUserRankArgs();

	SocialAsyncHandle handle = CreateHandle();

	// create controller

	SC_ScoresController_h controller;

	error = SC_Client_CreateScoresController(client, &controller, HandleScoreList, args);

	// set search criteria

	SC_ScoresSearchList_t searchList = MakeSearchList(days);
	SC_ScoresController_SetSearchList(controller, searchList);

	// todo: create scores controller, etc

	args->self = this;
	args->handle = handle;*/
}

SocialAsyncHandle SocialSC::AsyncChallengeNew(int userId)
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	return CreateHandle(); // todo
}

SocialAsyncHandle SocialSC::AsyncChallengeCancel(int challengeId)
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	return CreateHandle(); // todo
}

SocialAsyncHandle SocialSC::AsyncChallengeListPending()
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	return CreateHandle(); // todo
}

SocialAsyncHandle SocialSC::AsyncChallengeListCompleted()
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	return CreateHandle(); // todo
}

SocialAsyncHandle SocialSC::AsyncFriendAdd(int userId)
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	return CreateHandle(); // todo
}

SocialAsyncHandle SocialSC::AsyncFriendList()
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return CreateHandle();

	return CreateHandle(); // todo
}

void SocialSC::ScoreAcquire(SocialScore & score)
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return;

	if (score.m_user != NULL)
	{
		SC_User_Retain(score.m_user);
	}
}

void SocialSC::ScoreRelease(SocialScore & score)
{
	Assert(m_isInitialized);
	if (!m_isInitialized)
		return;

	if (score.m_user != NULL)
	{
		SC_User_Release(score.m_user);
		score.m_user = NULL;
	}
}

// SCUI

static bool g_uiIsInitialized = false;

bool SocialSCUI_Initialize()
{
	Assert(g_uiIsInitialized == false);
	Assert(g_isInitialized);

	SC_Error_t error = SC_OK;

	if (error == SC_OK && g_isInitialized == false)
	{
		error = SC_INVALID_STATE;
	}

	if (error == SC_OK && g_client == NULL)
	{
		error = SC_INVALID_STATE;
	}

	if (error == SC_OK)
	{
		error = SCUI_Client_New(&g_uiClient, g_client);
	}

	if (error == SC_OK && g_uiClient == NULL)
	{
		error = SC_INVALID_STATE;
	}

	if (error != SC_OK)
	{
		LOG_ERR("failed to create scoreloop UI client: %d", error);
		SocialSCUI_Shutdown();
		return false;
	}

	g_uiIsInitialized = true;

	return true;
}

void SocialSCUI_Shutdown()
{
	Assert(g_uiIsInitialized);

	if (g_uiClient != NULL)
	{
		SCUI_Client_Release(g_uiClient);
		g_uiClient = NULL;
	}

	g_uiIsInitialized = false;
}

static void HandleUIClosed(void * obj, SCUI_Result_t result, const void * data)
{
	if (result != SCUI_RESULT_OK)
	{
		LOG_ERR("scoreloop UI result: %d", result);
	}
	else
	{
		LOG_INF("scoreloop UI result: %d", result);
	}
}

void SocialSCUI_HandleEvent(bps_event_t * event)
{
	Assert(event != NULL);

	if (g_uiIsInitialized)
	{
		if (g_uiClient != NULL)
		{
			SCUI_Client_HandleEvent(g_uiClient, event);
		}
	}
}

void SocialSCUI_ShowUI()
{
	Assert(g_uiIsInitialized);
	if (!g_uiIsInitialized)
		return;

	SC_Error_t error;

	error = SCUI_Client_ShowFavoriteGamesView(g_uiClient, HandleUIClosed, NULL);

	if (error != SC_OK)
	{
		LOG_ERR("failed to show scoreloop UI: %d", error);
	}
	else
	{
		LOG_INF("scoreloop UI shown", 0);
	}
}
