#include <time.h>
#include "grs_types.h"
#include "Parse.h"
#include "StringEx.h"

namespace GRS
{
	void GameInfo::Setup(const std::string& name, int id, int version, const std::string& key)
	{
		m_Name = name;
		m_Id = id;
		m_Version = version;
		m_Key = key;
	}
	
	//
	
	CountryCode CountryCode::FromString(const std::string& text)
	{
		CountryCode result;
		
		result.m_Code = text;
		
		return result;
	}
	
	std::string CountryCode::ToString() const
	{
		return m_Code;
	}
	
	//
	
	GpsLocation::GpsLocation()
	{
		m_Location = 0.0f;
	}

	GpsLocation GpsLocation::FromString(const std::string& text)
	{
		GpsLocation result;
		
		if (text == String::Empty)
			return result;
		
		result.m_Location = Parse::Float(text);
		
		return result;
	}
	
	std::string GpsLocation::ToString() const
	{
		return String::Empty;
	}
	
	//
	
	TimeStamp TimeStamp::FromSystemTime()
	{
		TimeStamp result;
		
		result.m_TimeStamp = (uint64_t)time(0);
		
		return result;
	}
	
	std::string TimeStamp::ToString() const
	{
		return String::FormatC("%ld", m_TimeStamp);
	}
	
	//
	
	std::string UserId::ToString() const
	{
		return m_Text;
	}
	
	//
	
	HighScore::HighScore()
	{
		Initialize();
	}
	
	void HighScore::Initialize()
	{
		m_GameMode = -1;
		m_Score = 0.0f;
		m_Ishacked = false;
	}
	
	void HighScore::Setup(int gameMode, int history, UserId userId, const std::string& userName, const std::string& tag, float score, TimeStamp timeStamp, GpsLocation gpsLocation, CountryCode countryCode, bool isHacked)
	{
		m_GameMode = gameMode;
		m_History = history;
		m_UserId = userId;
		m_UserName = userName;
		m_Tag = tag;
		m_Score = score;
		m_TimeStamp = timeStamp;
		m_GpsLocation = gpsLocation;
		m_CountryCode = countryCode;
		m_Ishacked = isHacked;
	}
	
	//
	
	ListQuery::ListQuery()
	{
		Initialize();
	}
	
	void ListQuery::Initialize()
	{
		Setup(0, 0, 0, UserId(), 0, 0, 0, 0);
	}
	
	void ListQuery::Setup(int gameId, int gameMode, int history, UserId userId, int resultBegin, int resultCount, UserId* filterUserId, CountryCode* filterCountryCode)
	{
		m_GameId = gameId;
		m_GameMode = gameMode;
		m_History = history;
		m_UserId = userId;
		m_ResultBegin = resultBegin;
		m_ResultCount = resultCount;
		m_FilterUserId = filterUserId;
		m_FilterCountryCode = filterCountryCode;
	}
	
	//
	
	RankQuery::RankQuery()
	{
		Initialize();
	}
	
	void RankQuery::Initialize()
	{
		Setup(0, 0, 0.0f);
	}
	
	void RankQuery::Setup(int gameId, int gameMode, float score)
	{
		m_GameId = gameId;
		m_GameMode = gameMode;
		m_Score = score;
	}
	
	//
	
	QueryResult::QueryResult()
	{
		Initialize();
	}
	
	void QueryResult::Initialize()
	{
		m_RequestType = RequestType_Undefined;
		m_RequestResult = RequestResult_Ok;
		
		m_GameMode = -1;
		
		m_ScoreCount = -1;
		
		m_Scores.clear();
		
		m_RankCurr = -1;
		m_RankBest = -1;
	}
}
