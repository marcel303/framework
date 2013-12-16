#pragma once

#include "SocialAPI.h"

namespace Game
{
	class UsgSocialListener : public SocialListener
	{
	public:
		virtual void HandleLogin(SocialAsyncHandle handle, SocialResult result);
		virtual void HandleUserNameGet(SocialAsyncHandle handle, SocialResult result, std::string & userName);
		virtual void HandleScoreSubmit(SocialAsyncHandle handle, SocialResult result, int rank);
		virtual void HandleScoreListRange(SocialAsyncHandle handle, SocialResult result, SocialScoreList & scoreList);
		virtual void HandleScoreListAroundUser(SocialAsyncHandle handle, SocialResult result, SocialScoreList & scoreList);
		virtual void HandleScoreGetUserRank(SocialAsyncHandle handle, SocialResult result, int rank);
		virtual void HandleChallengeNew(SocialAsyncHandle handle, SocialResult result);
		virtual void HandleChallengeCancel(SocialAsyncHandle handle, SocialResult result);
		virtual void HandleChallengeListPending(SocialAsyncHandle handle, SocialResult result, SocialChallengeList & challenges);
		virtual void HandleChallengeListCompleted(SocialAsyncHandle handle, SocialResult result, SocialChallengeList & challenges);
		virtual void HandleFriendAdd(SocialAsyncHandle handle, SocialResult result);
		virtual void HandleFriendList(SocialAsyncHandle handle, SocialResult result, SocialFriendList & friends);
	};
}
