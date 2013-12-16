#pragma once

#include "ByteString.h"
#include "grs_types.h"

namespace GRS
{
	class Protocol
	{
	public:
		Protocol();
		
		ByteString Encode_QueryList(const ListQuery& query);
		ByteString Encode_QueryRank(const RankQuery& query);
		ByteString Encode_SubmitHighScore(const HighScore& highScore, const GameInfo& gameInfo);
		ByteString Encode_SubmitCrashLog(const char* message, const GameInfo& gameInfo);
		QueryResult Decode_QueryResult(const char* text);
		
		static ByteString Encrypt(const ByteString& bytes, const ByteString& pass);
		static ByteString Decrypt(const ByteString& bytes, const ByteString& pass);
		
		static std::string Encode(const ByteString& bytes);
		static ByteString Decode(const std::string& text);
		
		static void DBG_SelfTest();
	};
}
