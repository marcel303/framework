#include "Base64.h"
#include "Exception.h"
#include "grs.h"
#include "Log.h"
#include "StringEx.h"

namespace GRS
{
	void HttpClient::Initialize(const char* host, int port, const char* path, GameInfo gameInfo)
	{
		m_Exchange.OnResult = CallBack(this, HandlePostResult);
		
		m_Host = host;
		m_Port = port;
		m_Path = path;
		m_GameInfo = gameInfo;
	}
	
	void HttpClient::RequestHighScoreList(const ListQuery& query)
	{
		ByteString bytes1 = m_Protocol.Encode_QueryList(query);
		
//		LOG(LogLevel_Debug, bytes1.ToString().c_str());
		
		ByteString bytes2 = m_Protocol.Encrypt(bytes1, ByteString::FromString(m_GameInfo.m_Key.c_str()));
		
//		LOG(LogLevel_Debug, bytes2.ToString().c_str());
		
		//
		
		Post(bytes2, false);
	}
	
	void HttpClient::RequestHighScoreRank(const RankQuery& query)
	{
		ByteString bytes1 = m_Protocol.Encode_QueryRank(query);
		
		ByteString bytes2 = m_Protocol.Encrypt(bytes1, ByteString::FromString(m_GameInfo.m_Key.c_str()));
		
		Post(bytes2, false);
	}
	
	void HttpClient::SubmitHighScore(const HighScore& highScore)
	{
		ByteString bytes1 = m_Protocol.Encode_SubmitHighScore(highScore, m_GameInfo);
		
		LOG(LogLevel_Debug, bytes1.ToString().c_str());
		
		ByteString bytes2 = m_Protocol.Encrypt(bytes1, ByteString::FromString(m_GameInfo.m_Key.c_str()));
		
		Post(bytes2, false);
	}
	
	void HttpClient::SubmitException(const char* message)
	{
		ByteString bytes1 = m_Protocol.Encode_SubmitCrashLog(message, m_GameInfo);
		
		ByteString bytes2 = m_Protocol.Encrypt(bytes1, ByteString::FromString(m_GameInfo.m_Key.c_str()));
		
		Post(bytes2, true);
	}
	
	const GameInfo& HttpClient::GameInfo_get() const
	{
		return m_GameInfo;
	}
	
	const char* HttpClient::Path_get() const
	{
		return m_Path.c_str();
	}
	
	void HttpClient::Post(const ByteString& bytes, bool synchronized)
	{
		std::string base64 = m_Protocol.Encode(bytes);
		
//		LOG(LogLevel_Debug, base64.c_str());
		
		std::string httpBody = "message=" + Base64::HttpFix(base64);
		
		std::string url = String::FormatC("http://%s:%d/%s", m_Host.c_str(), m_Port, m_Path.c_str());
		
		if (synchronized)
			m_Exchange.Post_Synchronized(url, httpBody);
		else
			m_Exchange.Post(url, httpBody);
	}
	
	void HttpClient::HandlePostResult(void* obj, void* arg)
	{
		HttpClient* self = (HttpClient*)obj;
		
		std::string* text = (std::string*)arg;
		
		QueryResult result = self->m_Protocol.Decode_QueryResult(text->c_str());
		
		switch (result.m_RequestType)
		{
			case RequestType_QueryList:
			{
				if (self->OnQueryListResult.IsSet())
					self->OnQueryListResult.Invoke(&result);
				break;
			}
			case RequestType_QueryRank:
			{
				if (self->OnQueryRankResult.IsSet())
					self->OnQueryRankResult.Invoke(&result);
				break;
			}
			case RequestType_SubmitScore:
			{
				if (self->OnSubmitScoreResult.IsSet())
					self->OnSubmitScoreResult.Invoke(&result);
				break;
			}
			case RequestType_SubmitCrashLog:
			{
				if (self->OnSubmitCrashLogResult.IsSet())
					self->OnSubmitCrashLogResult.Invoke(&result);
				break;
			}
			case RequestType_Undefined:
			{
				LOG_WRN("grs: request type not set for post result", 0);
				break;
			}
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown request type: %d", (int)result.m_RequestType);
#else
				break;
#endif
		}
	}
}
