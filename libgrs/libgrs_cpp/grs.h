#pragma once

#include "ByteString.h"
#include "CallBack.h"
#include "FixedSizeString.h"
#include "grs_exchange.h"
#include "grs_protocol.h"
#include "grs_types.h"

#define GRS_MAX_HISTORY 1000

namespace GRS
{
	class HttpClient
	{
	public:
		void Initialize(const char* host, int port, const char* path, GameInfo gameInfo);
		void RequestHighScoreList(const ListQuery& query);
		void RequestHighScoreRank(const RankQuery& query);
		void SubmitHighScore(const HighScore& highScore);
		void SubmitException(const char* message);
		
		const GameInfo& GameInfo_get() const;
		const char* Path_get() const;
		
		CallBack OnQueryListResult;
		CallBack OnQueryRankResult;
		CallBack OnSubmitScoreResult;
		CallBack OnSubmitCrashLogResult;
		
	private:
		void Post(const ByteString& bytes, bool synchronized);
		static void HandlePostResult(void* obj, void* arg);
		
		FixedSizeString<128> m_Host;
		int m_Port;
		FixedSizeString<128> m_Path;
		GameInfo m_GameInfo;
		Protocol m_Protocol;
		Exchange m_Exchange;
	};		
}
