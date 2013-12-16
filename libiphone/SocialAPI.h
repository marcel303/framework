#pragma once

#include <string>
#include <vector>

enum SocialResult
{
	kSocialOK,
	kSocialError
};

class SocialAsyncHandle
{
public:
	int m_id;
};

class SocialScore
{
public:
	SocialScore() { }
	SocialScore(int mode, int level, int value, int rank, std::string userName)
		: m_mode(mode)
		, m_level(level)
		, m_value(value)
		, m_rank(rank)
		, m_userName(userName)
	{
		#ifdef BBOS
		m_user = NULL;
		#endif
	}

	int m_mode;
	int m_level;
	int m_value;
	int m_rank;
	std::string m_userName;

	#ifdef BBOS
	struct SC_User_tag * m_user;
	#endif
};

class SocialScoreList
{
public:
	std::vector<SocialScore> scores;
	int scoreCount; // -1 if unknown
};

class SocialChallenge
{
public:
	int m_challengeId;
	int m_userId;
};

class SocialChallengeList
{
public:
	std::vector<SocialChallenge> challenges;
};

class SocialFriend
{
public:
	int m_userId;
	std::string m_userName;
};

class SocialFriendList
{
public:
	std::vector<SocialFriend> friends;
};

class SocialListener
{
public:
	virtual ~SocialListener() { }

	virtual void HandleLogin(SocialAsyncHandle handle, SocialResult result) = 0;
	virtual void HandleUserNameGet(SocialAsyncHandle handle, SocialResult result, std::string & userName) = 0;
	virtual void HandleScoreSubmit(SocialAsyncHandle handle, SocialResult result, int rank) = 0;
	virtual void HandleScoreListRange(SocialAsyncHandle handle, SocialResult result, SocialScoreList & scoreList) = 0;
	virtual void HandleScoreListAroundUser(SocialAsyncHandle handle, SocialResult result, SocialScoreList & scoreList) = 0;
	virtual void HandleScoreGetUserRank(SocialAsyncHandle handle, SocialResult result, int rank) = 0;
	virtual void HandleChallengeNew(SocialAsyncHandle handle, SocialResult result) = 0;
	virtual void HandleChallengeCancel(SocialAsyncHandle handle, SocialResult result) = 0;
	virtual void HandleChallengeListPending(SocialAsyncHandle handle, SocialResult result, SocialChallengeList & challenges) = 0;
	virtual void HandleChallengeListCompleted(SocialAsyncHandle handle, SocialResult result, SocialChallengeList & challenges) = 0;
	virtual void HandleFriendAdd(SocialAsyncHandle handle, SocialResult result) = 0;
	virtual void HandleFriendList(SocialAsyncHandle handle, SocialResult result, SocialFriendList & friends) = 0;
};

class SocialAPI
{
public:
	virtual ~SocialAPI() { }

	virtual bool Initialize(SocialListener * listener) = 0;
	virtual void Shutdown() = 0;
	virtual void Process() = 0;

	virtual bool IsLoggedIn() = 0;
	virtual SocialAsyncHandle AsyncLogin() = 0;
	virtual SocialAsyncHandle AsyncLoginNameGet() = 0;
	virtual SocialAsyncHandle AsyncScoreSubmit(int mode, int level, int value) = 0;
	virtual SocialAsyncHandle AsyncScoreListRange(int mode, int days, int rangeBegin, int rangeCount) = 0;
	virtual SocialAsyncHandle AsyncScoreListAroundUser(int mode, int days, int count) = 0;
	virtual SocialAsyncHandle AsyncScoreGetUserRank(int mode, int days) = 0;
	virtual SocialAsyncHandle AsyncChallengeNew(int userId) = 0;
	virtual SocialAsyncHandle AsyncChallengeCancel(int challengeId) = 0;
	virtual SocialAsyncHandle AsyncChallengeListPending() = 0;
	virtual SocialAsyncHandle AsyncChallengeListCompleted() = 0;
	virtual SocialAsyncHandle AsyncFriendAdd(int userId) = 0;
	virtual SocialAsyncHandle AsyncFriendList() = 0;

	virtual void ScoreAcquire(SocialScore & score) = 0;
	virtual void ScoreRelease(SocialScore & score) = 0;
};
