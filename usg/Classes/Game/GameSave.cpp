#include "Archive.h"
#include "Base64.h"
#include "ByteString.h"
#include "EntityPlayer.h"
#include "FileStream.h"
#include "GameRound.h"
#include "GameSave.h"
#include "GameScore.h"
#include "GameState.h"
#if defined(IPHONEOS)
#include "grs.h"
#endif
#include "MemoryStream.h"
#include "Parse.h"
#if defined(PSP)
#include "PspSaveData.h"
#endif
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"
#include "System.h"
#include "World.h"

#define SAVE_PATH g_System.GetDocumentPath("save.dat")
#define SAVE_PASS "z1y3x5v7u9l1p3q5"

namespace Game
{
	GameSave::GameSave()
	{
	}
	
	void GameSave::Save()
	{
		Assert(g_GameState->m_GameRound->GameModeIsClassic());
		
		MemoryStream memoryStream;
		
		// open archive
		
		Archive a;
		
		a.OpenWrite(&memoryStream);
		
		// save game settings
		
		a.WriteSection("game");
		a.WriteValue_Int32("difficulty", (int)g_GameState->m_GameRound->Modifier_Difficulty_get());
		a.WriteValue_Int32("score", g_GameState->m_Score->Score_get());
		a.WriteValue_Int32("level", g_GameState->m_GameRound->Classic_Level_get());
		a.WriteSectionEnd();
		
		// save player
		
		a.WriteSection("player");
		g_World->m_Player->Save(a);
		a.WriteSectionEnd();
		
		// save world
		
		a.WriteSection("world");
		g_World->Save(a);
		a.WriteSectionEnd();
		
		// close archive
		
		a.WriteEOF();
		a.Close();
		
		// convert memory string to byte string
		
		memoryStream.Seek(0, SeekMode_Begin);
		
		int length = memoryStream.Length_get();

#if defined(PSP)
		PspSaveData_Save(PSPSAVE_APPNAME, PSPSAVE_CONTINUE, PSPSAVE_DESC, PSPSAVE_DESC_LONG, memoryStream.Bytes_get(), memoryStream.Length_get(), true);
#else
		ByteString bytes(length);
		
		memoryStream.Read(&bytes.m_Bytes[0], length);
		
#if defined(IPHONEOS)
		// encrypt bytes

		bytes = GRS::Protocol::Encrypt(bytes, ByteString::FromString(SAVE_PASS));
		
		// encode string
		
		std::string text = GRS::Protocol::Encode(bytes);
#else
		std::string text = Base64::Encode(bytes);
#endif
		
		// write string to disk
		
		std::string path = SAVE_PATH;
		
		FileStream stream(path.c_str(), OpenMode_Write);
		
		StreamWriter writer(&stream, false);
		
		writer.WriteText_Binary(text);
#endif
	}

	static void HandleSection(Archive& src, Archive& dst)
	{
		while (src.Read())
		{
			if (src.TypeIsSection())
			{
				dst.WriteSection(src.GetSection());
				HandleSection(src, dst);
			}
			else if (src.TypeIsSectionEnd())
			{
				dst.WriteSectionEnd();
				return;
			}
			else if (src.TypeIsValue())
			{
				if (src.IsKey("lives"))
					dst.WriteValue_Int32("lives", g_World->m_Player->Lives_get());
				else
					dst.WriteValue(src.GetKey(), src.GetValue());
			}
			else if (src.TypeIsEOF())
			{
				dst.WriteEOF();
				return;
			}
			else
			{
#ifndef DEPLOYMENT
				throw ExceptionNA();
#endif
			}
		}
	}

	void GameSave::SaveUpdate()
	{
		if (!HasSave_get())
			return;

		// read string from disk
		
#if defined(PSP)
		uint8_t temp[65536];
		int byteCount = sizeof(temp);

		PspSaveData_Load(PSPSAVE_APPNAME, PSPSAVE_CONTINUE, temp, byteCount, byteCount);

		MemoryStream memoryStreamSrc(temp, byteCount);
		MemoryStream memoryStreamDst;
#else
		std::string path = SAVE_PATH;
		
		FileStream stream(path.c_str(), OpenMode_Read);
		
		StreamReader reader(&stream, false);
		
		std::string text = reader.ReadText_Binary();
		
		stream.Close();

#if defined(IPHONEOS)
		// decode string
		
		ByteString bytes = GRS::Protocol::Decode(text);
		
		// decrypt string
		
		bytes = GRS::Protocol::Decrypt(bytes, ByteString::FromString(SAVE_PASS));
#else
		ByteString bytes = Base64::Decode(text);
#endif

		// move to memory stream

		MemoryStream memoryStreamSrc;
		MemoryStream memoryStreamDst;

		memoryStreamSrc.Write(&bytes.m_Bytes[0], bytes.Length_get());
		memoryStreamSrc.Seek(0, SeekMode_Begin);
#endif

		// update archive

		Archive src;
		Archive dst;

		src.OpenRead(&memoryStreamSrc);
		dst.OpenWrite(&memoryStreamDst);

		HandleSection(src, dst);

		Assert(memoryStreamDst.Position_get() == memoryStreamSrc.Position_get());
		
#if defined(PSP)
		PspSaveData_Save(PSPSAVE_APPNAME, PSPSAVE_CONTINUE, PSPSAVE_DESC, PSPSAVE_DESC_LONG, memoryStreamDst.Bytes_get(), memoryStreamDst.Length_get(), true);
#else
		int length = memoryStreamDst.Length_get();

		bytes.Length_set(length);
		
		memoryStreamDst.Seek(0, SeekMode_Begin);
		memoryStreamDst.Read(&bytes.m_Bytes[0], length);

#if defined(IPHONEOS)
		bytes = GRS::Protocol::Encrypt(bytes, ByteString::FromString(SAVE_PASS));
		
		// encode string
		
		text = GRS::Protocol::Encode(bytes);
#else
		text = bytes.ToString();
#endif
		
		// write string to disk
		
		stream.Open(path.c_str(), OpenMode_Write);
		
		StreamWriter writer(&stream, false);
		
		writer.WriteText_Binary(text);

		stream.Close();
#endif
	}
	
	void GameSave::Load()
	{
		// read string from disk
		
		std::string path = SAVE_PATH;
		
#if defined(PSP)
		uint8_t bytes2[65536];
		int byteCount = sizeof(bytes2);

		PspSaveData_Load(PSPSAVE_APPNAME, PSPSAVE_CONTINUE, bytes2, byteCount, byteCount);

		MemoryStream memoryStream(bytes2, byteCount);
#else
		FileStream stream(path.c_str(), OpenMode_Read);
		
		StreamReader reader(&stream, false);
		
		std::string text = reader.ReadText_Binary();
		
//		LOG(LogLevel_Debug, "load: %s", text.c_str());

#if defined(IPHONEOS)
		// decode string
		
		ByteString bytes = GRS::Protocol::Decode(text);
		
		// decrypt string
		
		bytes = GRS::Protocol::Decrypt(bytes, ByteString::FromString(SAVE_PASS));
#else
		ByteString bytes = Base64::Decode(text);
#endif
		
		MemoryStream memoryStream(&bytes.m_Bytes[0], bytes.Length_get());
#endif
		
		Archive a;
		
		a.OpenRead(&memoryStream);
		
		while (a.NextSection())
		{
			if (a.IsSection("game"))
			{
				while (a.NextValue())
				{
					if (a.IsKey("difficulty"))
						g_GameState->m_GameRound->LOAD_Difficulty_set((Difficulty)a.GetValue_Int32());
					else if (a.IsKey("score"))
						g_GameState->m_Score->Score_set(a.GetValue_Int32());
					else if (a.IsKey("level"))
						g_GameState->m_GameRound->LOAD_Level_set(a.GetValue_Int32());
					else
					{
#ifndef DEPLOYMENT
						throw ExceptionVA("unknown key: %s", a.GetKey());
#endif
					}
				}
			}
			else if (a.IsSection("player"))
			{
				g_World->m_Player->Load(a);
			}
			else if (a.IsSection("world"))
			{
				g_World->Load(a);
			}
			else
			{
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown section: %s", a.GetSection());
#endif
			}

		}
	}
	
	void GameSave::Clear()
	{
		try
		{
			if (HasSave_get())
			{
				std::string path = SAVE_PATH;

#if defined(PSP)
				PspSaveData_Remove(PSPSAVE_APPNAME, PSPSAVE_CONTINUE);
#else
				FileStream::Delete(path.c_str());
#endif
				
				Assert(!HasSave_get());
			}
		}
		catch (std::exception& e)
		{
			LOG(LogLevel_Warning, "game save clear failed: %s", e.what());
		}
	}
	
	bool GameSave::HasSave_get() const
	{
#if defined(PSP)
		return PspSaveData_Exists(PSPSAVE_APPNAME, PSPSAVE_CONTINUE);
#else
		std::string path = SAVE_PATH;
		
		return FileStream::Exists(path.c_str());
#endif
	}
}
