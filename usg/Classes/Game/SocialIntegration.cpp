#include "GameState.h"
#include "Log.h"
#include "SocialIntegration.h"
#include "View_Scores.h"
#include "View_ScoreSubmit.h"

namespace Game
{
	void UsgSocialListener::HandleLogin(SocialAsyncHandle handle, SocialResult result)
	{
		if (result != kSocialOK)
		{
			LOG_ERR("social: failed to login", 0);
		}
		else
		{
			LOG_INF("social: logged in", 0);
		}
	}

	void UsgSocialListener::HandleUserNameGet(SocialAsyncHandle handle, SocialResult result, std::string & userName)
	{
	}

	void UsgSocialListener::HandleScoreSubmit(SocialAsyncHandle handle, SocialResult result, int rank)
	{
		if (result != kSocialOK)
		{
			LOG_ERR("social: failed to submit score", 0);

			View_ScoreSubmit * view = (View_ScoreSubmit*)g_GameState->GetView(::View_ScoreSubmit);

			view->Handle_GameCenterSubmitFailed();
		}
		else
		{
			LOG_INF("social: score submitted", 0);

			View_ScoreSubmit * view = (View_ScoreSubmit*)g_GameState->GetView(::View_ScoreSubmit);

			view->Handle_GameCenterSubmitComplete(rank);
		}
	}

	void UsgSocialListener::HandleScoreListRange(SocialAsyncHandle handle, SocialResult result, SocialScoreList & scoreList)
	{
		if (result != kSocialOK)
		{
			LOG_ERR("social: failed to list scores", 0);
		}
		else
		{
			View_Scores * view = (View_Scores*)g_GameState->GetView(::View_Scores);

			view->HandleScoreList(scoreList, false);
		}
	}

	void UsgSocialListener::HandleScoreListAroundUser(SocialAsyncHandle handle, SocialResult result, SocialScoreList & scoreList)
	{
		if (result != kSocialOK)
		{
			LOG_ERR("social: failed to list scores", 0);
		}
		else
		{
			View_Scores * view = (View_Scores*)g_GameState->GetView(::View_Scores);

			view->HandleScoreList(scoreList, true);
		}
	}

	void UsgSocialListener::HandleScoreGetUserRank(SocialAsyncHandle handle, SocialResult result, int rank)
	{
	}

	void UsgSocialListener::HandleChallengeNew(SocialAsyncHandle handle, SocialResult result)
	{
	}

	void UsgSocialListener::HandleChallengeCancel(SocialAsyncHandle handle, SocialResult result)
	{
	}

	void UsgSocialListener::HandleChallengeListPending(SocialAsyncHandle handle, SocialResult result, SocialChallengeList & challenges)
	{
	}

	void UsgSocialListener::HandleChallengeListCompleted(SocialAsyncHandle handle, SocialResult result, SocialChallengeList & challenges)
	{
	}

	void UsgSocialListener::HandleFriendAdd(SocialAsyncHandle handle, SocialResult result)
	{
	}

	void UsgSocialListener::HandleFriendList(SocialAsyncHandle handle, SocialResult result, SocialFriendList & friends)
	{
	}
}
