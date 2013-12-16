#pragma once

#include <string>
#include <vector>
#include <stdint.h>

namespace GRS
{
	class GameInfo
	{
	public:
		void Setup(const std::string& name, int id, int version, const std::string& key);
		
		std::string m_Name;
		int m_Id;
		int m_Version;
		std::string m_Key;
	};
	
	class CountryCode
	{
	public:
		static CountryCode FromString(const std::string& text);
		std::string ToString() const;
		
		std::string m_Code;
	};
	
	class GpsLocation
	{
	public:
		GpsLocation();

		static GpsLocation FromString(const std::string& text);
		std::string ToString() const;
		
		// todo
		float m_Location;
	};
	
	class TimeStamp
	{
	public:
		static TimeStamp FromSystemTime();
		std::string ToString() const;
		
		uint64_t m_TimeStamp;
	};
	
	class UserId
	{
	public:
		UserId()
		{
		}
		
		UserId(const std::string& text)
		{
			m_Text = text;
		}
		
		std::string ToString() const;
		
		std::string m_Text;
	};
	
	class HighScore
	{
	public:
		HighScore();
		void Initialize();
		void Setup(int gameMode, int history, UserId userId, const std::string& userName, const std::string& tag, float score, TimeStamp timeStamp, GpsLocation gpsLocation, CountryCode countryCode, bool isHacked);
		
		int m_GameMode;
		int m_History;
		UserId m_UserId;
		std::string m_UserName;
		std::string m_Tag;
		float m_Score;
		TimeStamp m_TimeStamp;
		GpsLocation m_GpsLocation;
		CountryCode m_CountryCode;
		bool m_Ishacked;
	};
	
	enum RequestType
	{
		RequestType_Undefined,
		RequestType_QueryList, // retrieve a list of high scores
		RequestType_QueryRank, // retrieve the position a given high score would rank
		RequestType_SubmitScore, // submit a high score entry
		RequestType_SubmitCrashLog // submit a crash log
	};
	
	enum RequestResult
	{
		RequestResult_Ok,
		RequestResult_Error
	};
	
	class ListQuery
	{
	public:
		ListQuery();
		void Initialize();
		void Setup(int gameId, int gameMode, int history, UserId userId, int resultBegin, int resultCount, UserId* filterUserId, CountryCode* filterCountryCode);
		
		int m_GameId;
		int m_GameMode;
		int m_History;
		UserId m_UserId;
		
		// query result
		int m_ResultBegin;
		int m_ResultCount;
		
		UserId* m_FilterUserId;
		CountryCode* m_FilterCountryCode;
	};
	
	class RankQuery
	{
	public:
		RankQuery();
		void Initialize();
		void Setup(int gameId, int gameMode, float score);
		
		int m_GameId;
		int m_GameMode;
		float m_Score;
	};
	
	class QueryResult
	{
	public:
		QueryResult();
		void Initialize();
		
		RequestType m_RequestType;
		RequestResult m_RequestResult;
		
		// query result
		
		int m_GameMode;
		int m_ScoreCount;
		std::vector<HighScore> m_Scores;
		int m_ResultBegin;
		
		// query rank result
		
		int m_Position;
		
		// submit result
		
		int m_RankCurr;
		int m_RankBest;
	};
}
