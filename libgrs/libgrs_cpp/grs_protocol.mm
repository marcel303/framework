#ifdef IPHONEOS
	#include <CommonCrypto/CommonCryptor.h>
	#include <Foundation/Foundation.h>
#endif
#include <stdlib.h>
#include "Base64.h"
#include "Debugging.h"
#include "Exception.h"
#include "grs_protocol.h"
#include "StringEx.h"

#define ALGORITHM kCCAlgorithmAES128
#define BLOCKSIZE kCCBlockSizeAES128
#define KEYSIZE kCCKeySizeAES128
#define OPTIONS 0

#ifdef IPHONEOS

@interface QueryResultReader : NSObject <NSXMLParserDelegate>
{
	@private
	
	GRS::QueryResult result;
}

-(GRS::QueryResult)getResult;

@end

@implementation QueryResultReader

- (void)parser:(NSXMLParser *)parser parseErrorOccurred:(NSError*)parseError 
{
#if !defined(DEPLOYMENT) && 0
	NSString* errorString = [NSString stringWithFormat:@"Failed to read GRS query result from XML (%i)", [parseError code]];
	
	UIAlertView * alert = [[UIAlertView alloc] initWithTitle:@"Error" message:errorString delegate:self cancelButtonTitle:@"OK" otherButtonTitles:nil];
	
	[alert show];
#endif
}

- (void)parser:(NSXMLParser*)parser didStartElement:(NSString*)elementName namespaceURI:(NSString*)namespaceURI qualifiedName:(NSString*)qName attributes:(NSDictionary*)attributeDict
{
	if ([elementName isEqualToString:@"message"])
	{
		NSString* requestAttribute = [attributeDict valueForKey:@"request"];
		
#ifndef DEPLOYMENT
		if (requestAttribute == nil)
			throw ExceptionVA("missing request type in response");
#else
		if (requestAttribute == nil)
			return;
#endif
		
		if ([requestAttribute isEqualToString:@"query_list"])
			result.m_RequestType = GRS::RequestType_QueryList;
		else if ([requestAttribute isEqualToString:@"query_rank"])
			result.m_RequestType = GRS::RequestType_QueryRank;
		else if ([requestAttribute isEqualToString:@"submit_score"])
			result.m_RequestType = GRS::RequestType_SubmitScore;
		else if ([requestAttribute isEqualToString:@"submit_crashlog"])
			result.m_RequestType = GRS::RequestType_SubmitCrashLog;
		else
		{
#ifndef DEPLOYMENT
			throw ExceptionVA("unknown request type in response: %s", [requestAttribute cStringUsingEncoding:NSASCIIStringEncoding]);
#else
			result.m_RequestType = GRS::RequestType_Undefined;
#endif
		}
	}
	
	if ([elementName isEqualToString:@"query"])
	{
		NSString* gameModeAttribute = [attributeDict valueForKey:@"game_mode"];
		NSString* scoreCountAttribute = [attributeDict valueForKey:@"score_count"];
		NSString* beginAttribute = [attributeDict valueForKey:@"begin"];
		NSString* bestRankAttribute = [attributeDict valueForKey:@"best_rank"];

#ifndef DEPLOYMENT
		if (gameModeAttribute == nil)
			throw ExceptionVA("missing game_mode attribute in response");
		if (beginAttribute == nil)
			throw ExceptionVA("missing begin attribute in response");
		if (bestRankAttribute == nil)
			throw ExceptionVA("missing best rank attribute in response");
#else
		if (gameModeAttribute == nil)
			return;
		if (beginAttribute == nil)
			return;
		if (bestRankAttribute == nil)
			return;
#endif
		
		if (scoreCountAttribute == nil)
			scoreCountAttribute = @"-1";
		
		int gameMode = [gameModeAttribute intValue];
		int scoreCount = [scoreCountAttribute intValue];
		int begin = [beginAttribute intValue];
		int bestRank = [bestRankAttribute intValue];
		
		result.m_GameMode = gameMode;
		result.m_ScoreCount = scoreCount;
		result.m_ResultBegin = begin;
		result.m_RankBest = bestRank;
	}
	
	if ([elementName isEqualToString:@"submit"])
	{
		NSString* currRankAttribute = [attributeDict valueForKey:@"curr_rank"];
		NSString* bestRankAttribute = [attributeDict valueForKey:@"best_rank"];
		
		if (currRankAttribute != nil)
			result.m_RankCurr = [currRankAttribute intValue];
		if (bestRankAttribute != nil)
			result.m_RankBest = [bestRankAttribute intValue];
	}
	
	if ([elementName isEqualToString:@"score"]) 
	{
		NSString* nameAttribute = [attributeDict valueForKey:@"user_name"];
		NSString* tagAttribute = [attributeDict valueForKey:@"tag"];
		NSString* scoreAttribute = [attributeDict valueForKey:@"score"];
		NSString* gpsLocationAttribute = [attributeDict valueForKey:@"gps_location"];
		NSString* countryCodeAttribute = [attributeDict valueForKey:@"country_code"];
		
#ifndef DEPLOYMENT
		if (nameAttribute == nil)
			throw ExceptionVA("missing name attribute in response");
		if (scoreAttribute == nil)
			throw ExceptionVA("missing score attribute in response");
#else
		if (nameAttribute == nil)
			return;
		if (scoreAttribute == nil)
			return;
#endif
		
		if (tagAttribute == nil)
			tagAttribute = @"";
		if (gpsLocationAttribute == nil)
			gpsLocationAttribute = @"";
		if (countryCodeAttribute == nil)
			countryCodeAttribute = @"";
		
		//
		
		std::string name = [nameAttribute cStringUsingEncoding:NSASCIIStringEncoding];
		std::string tag = [tagAttribute cStringUsingEncoding:NSASCIIStringEncoding];
		float score = [scoreAttribute floatValue];
		GRS::TimeStamp timeStamp = GRS::TimeStamp::FromSystemTime();
		GRS::GpsLocation gpsLocation = GRS::GpsLocation::FromString([gpsLocationAttribute cStringUsingEncoding:NSASCIIStringEncoding]);
		GRS::CountryCode countryCode = GRS::CountryCode::FromString([countryCodeAttribute cStringUsingEncoding:NSASCIIStringEncoding]);
		
		GRS::HighScore highScore;
		
		highScore.Setup(result.m_GameMode, 0, GRS::UserId(), name, tag, score, timeStamp, gpsLocation, countryCode, false);
		
		result.m_Scores.push_back(highScore);
	}
	
	if ([elementName isEqualToString:@"rank"])
	{
		NSString* positionAttribute = [attributeDict valueForKey:@"position"];
		
#ifndef DEPLOYMENT
		if (positionAttribute == nil)
			throw ExceptionVA("missing position attribute in response");
#else
		if (positionAttribute == nil)
			return;
#endif
		
		int position = [positionAttribute intValue];
		
		result.m_Position = position;
	}
}

-(GRS::QueryResult)getResult
{
	return result;
}

@end

#endif

namespace GRS
{
	Protocol::Protocol()
	{
	}
	
	ByteString Protocol::Encode_QueryList(const ListQuery& query)
	{
		std::string userIdString = query.m_UserId.ToString();
		const char* userIdString2 = userIdString.c_str();
	
		std::string message1 = "<message request=\"query_list_hist\">";
		std::string filter = String::Format(
			"<filter"\
			" user_id=\"%s\""\
			" game_id=\"%d\""\
			" game_mode=\"%d\""\
			" hist=\"%d\""\
			" row_begin=\"%d\""\
			" row_count=\"%d\"",
			userIdString2,
			query.m_GameId, query.m_GameMode, query.m_History, query.m_ResultBegin, query.m_ResultCount);
		if (query.m_FilterUserId)
			filter += String::Format(" user_id=\"%s\"", query.m_FilterUserId->ToString().c_str());
		if (query.m_FilterCountryCode)
			filter += String::Format(" country_code=\"%s\"", query.m_FilterCountryCode->ToString().c_str());
		filter += " />";
		std::string message2 = "</message>";
		
		std::string text = message1 + filter + message2;
		
		return ByteString::FromString(text);
	}
	
	ByteString Protocol::Encode_QueryRank(const RankQuery& query)
	{
		// game id, game mode, score
		
		std::string message1 = "<message request=\"query_rank\">";
		std::string score = String::Format("<score game_id=\"%d\" game_mode=\"%d\" score=\"%f\"/>", query.m_GameId, query.m_GameMode, query.m_Score);
		std::string message2 = "</message>";
		
		std::string text = message1 + score + message2;
		
		return ByteString::FromString(text);
	}
	
	ByteString Protocol::Encode_SubmitHighScore(const HighScore& highScore, const GameInfo& gameInfo)
	{
		std::string message1 = "<message request=\"submit_score\">";
		std::string score = String::Format(
			"<score"\
			" game_id=\"%d\""\
			" game_version=\"%d\""\
			" game_mode=\"%d\""\
			" date=\"%s\""\
			" user_id=\"%s\""\
			" score=\"%f\""\
			" user_name=\"%s\""\
			" tag=\"%s\""\
			" country_code=\"%s\""\
			" is_hacked=\"%d\""\
			" history=\"%d\""\
			"/>",
			gameInfo.m_Id,
			gameInfo.m_Version,
			highScore.m_GameMode,
			highScore.m_TimeStamp.ToString().c_str(),
			highScore.m_UserId.ToString().c_str(),
			highScore.m_Score,
			highScore.m_UserName.c_str(),
			highScore.m_Tag.c_str(),
			highScore.m_CountryCode.ToString().c_str(),
			highScore.m_Ishacked ? 1 : 0,
			highScore.m_History);
		std::string message2 = "</message>";
		std::string text = message1 + score + message2;

		return ByteString::FromString(text);
	}
	
	ByteString Protocol::Encode_SubmitCrashLog(const char* message, const GameInfo& gameInfo)
	{
		std::string message1 = "<message request=\"submit_crashlog\">";
		std::string score = String::Format(
			"<exception"\
			" game_id=\"%d\""\
			" game_version=\"%d\""\
			" date=\"%s\""\
			" message=\"%s\""\
			"/>",
			gameInfo.m_Id,
			gameInfo.m_Version,
			TimeStamp::FromSystemTime().ToString().c_str(),
			message);
		std::string message2 = "</message>";
		
		std::string text = message1 + score + message2;
		
		return ByteString::FromString(text);
	}
	
	QueryResult Protocol::Decode_QueryResult(const char* text)
	{
#ifdef IPHONEOS
		// read XML
		
//		NSString* string = [NSString stringWithCString:text length:strlen(text) encoding:NSASCIIStringEncoding];
		NSString* string = [NSString stringWithCString:text encoding:NSASCIIStringEncoding];
		NSData* data = [string dataUsingEncoding:NSASCIIStringEncoding];
		NSXMLParser* parser = [[NSXMLParser alloc] initWithData:data];
		QueryResultReader* reader = [[QueryResultReader alloc] autorelease];
		
		[parser setDelegate:reader];
		[parser setShouldProcessNamespaces:NO];
		[parser setShouldReportNamespacePrefixes:NO];
		[parser setShouldResolveExternalEntities:NO];
		[parser parse];
		
		return [reader getResult];
#elif defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(PSP)
		QueryResult result;

		return result;
#else
	#error
#endif
	}

#ifdef IPHONEOS
	static void MakePass(const ByteString& pass, int length, uint8_t* out_Pass)
	{
		for (int i = 0; i < pass.Length_get() && i < length; ++i)
			out_Pass[i] = pass.m_Bytes[i];
		
		for (int i = pass.Length_get(); i < length; ++i)
			out_Pass[i] = 0;
	}
#endif
	
	ByteString Protocol::Encrypt(const ByteString& bytes, const ByteString& pass)
	{
#ifdef IPHONEOS
		uint8_t passFixed[KEYSIZE];
		
		MakePass(pass, KEYSIZE, passFixed);
		
		ByteString ivBytes = ByteString::FromString("1234567890123456");
		uint8_t* iv = (uint8_t*)&ivBytes.m_Bytes[0];
		
		int srcSize = bytes.Length_get();
		int dstSize = 0;
		if (srcSize % BLOCKSIZE == 0)
			dstSize = srcSize;
		else
			dstSize = (srcSize / BLOCKSIZE + 1) * BLOCKSIZE;
		
		ByteString src = bytes;
		
		// perform padding using zeroes
		
		for (int i = srcSize; i < dstSize; ++i)
			src.m_Bytes.push_back(0);
		srcSize = dstSize;
		
		ByteString dst(dstSize);
		size_t byteCount = 0;
		
		CCCryptorStatus result = CCCrypt(kCCEncrypt, ALGORITHM, OPTIONS, passFixed, KEYSIZE,
			iv, // IV
			&src.m_Bytes[0], src.Length_get(),
			&dst.m_Bytes[0], dst.Length_get(),
			&byteCount);
		
#if DEBUG
		Assert(result == kCCSuccess);
#else
		if (result != kCCSuccess)
			LOG(LogLevel_Error, "cc failed");
#endif
		
		for (int i = byteCount; i < dstSize; ++i)
			dst.m_Bytes[i] = 0;
		
		return dst;
#elif defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(PSP)
		return bytes;
#else
	#error
#endif
	}
	
	ByteString Protocol::Decrypt(const ByteString& bytes, const ByteString& pass)
	{
#ifdef IPHONEOS
		uint8_t passFixed[KEYSIZE];
		
		MakePass(pass, KEYSIZE, passFixed);
		
		ByteString ivBytes = ByteString::FromString("1234567890123456");
		uint8_t* iv = (uint8_t*)&ivBytes.m_Bytes[0];
		
		size_t byteCount = 0;
		
		const ByteString& src = bytes;
		ByteString dst(src.Length_get() + BLOCKSIZE);
		
		CCCryptorStatus result = CCCrypt(kCCDecrypt, ALGORITHM, OPTIONS, passFixed, KEYSIZE,
			iv, // IV
			&src.m_Bytes[0], src.Length_get(),
			&dst.m_Bytes[0], dst.Length_get(),
			&byteCount);
		
#if DEBUG
		Assert(result == kCCSuccess);
#else
		if (result != kCCSuccess)
			LOG(LogLevel_Error, "cc failed");
#endif
		
		return dst.Extract(0, byteCount);
#else
		return bytes;
#endif
	}
	
	std::string Protocol::Encode(const ByteString& bytes)
	{
		return Base64::Encode(bytes);
	}
	
	ByteString Protocol::Decode(const std::string& text)
	{
		return Base64::Decode(text);
	}
	
	void Protocol::DBG_SelfTest()
	{
		Protocol protocol;
		
		// test encoding
		
		for (int i = 0; i < 1000; ++i)
		{
			ByteString bytes1;
			
			for (int j = 0; j < 100; ++j)
			{
				bytes1.m_Bytes.push_back(rand() & 255);
			}
			
			std::string text1 = protocol.Encode(bytes1);
			
			ByteString bytes2 = protocol.Decode(text1);
			
			Assert(bytes1.Equals(bytes2));
		}
		
		// test encryption
		
		for (int i = 0; i < 1000; ++i)
		{
			std::string pass;
			
			for (int j = 0; j < 32; ++j)
				pass.push_back(rand() & 255);
			
			ByteString passBytes = ByteString::FromString(pass);
			
			ByteString bytes1;
			
			for (int j = 0; j < 100; ++j)
			{
				bytes1.m_Bytes.push_back(rand() & 255);
			}
			
			ByteString bytes2 = protocol.Encrypt(bytes1, passBytes);
			
			ByteString bytes3 = protocol.Decrypt(bytes2, passBytes);
			
			Assert(bytes1.Equals(bytes3));
		}
		
		// test encoding
		
		// test decoding
	}
}
