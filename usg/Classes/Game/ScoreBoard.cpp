#include "FileStream.h"
#include "ScoreBoard.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "StringEx.h"
#include "System.h"

#define SAVE_PATH(gameMode) g_System.GetDocumentPath(String::FormatC("scores[%d].dat", gameMode).c_str())
#define VERSION 1

namespace Game
{
	static void Sort(Score* scores, int scoreCount);
	
	void Score::Setup(const std::string& _name, float _score)
	{
		name = _name;
		score = _score;
	}
	
	void Score::Load(Stream* stream)
	{
		StreamReader reader(stream, false);
		
		name = reader.ReadText_Binary();
		score = reader.ReadFloat();
	}
	
	void Score::Save(Stream* stream)
	{
		StreamWriter writer(stream, false);
		
		writer.WriteText_Binary(name);
		writer.WriteFloat(score);
	}
	
	//
	
	void ScoreFileHeader::Setup(int _version, int _gameMode, int _scoreCount)
	{
		version = _version;
		gameMode = _gameMode;
		scoreCount = _scoreCount;
	}

	void ScoreFileHeader::Load(Stream* stream)
	{
		StreamReader reader(stream, false);
		
		version = reader.ReadInt32();
		
		if (version == VERSION)
		{
			gameMode = reader.ReadInt32();
			scoreCount = reader.ReadInt32();
		}
		else
		{
			// note: must throw, so the file gets automaticalled restored
			
			throw ExceptionVA("unknown version: %d", version);
		}
	}
	
	void ScoreFileHeader::Save(Stream* stream)
	{
		StreamWriter writer(stream, false);
		
		writer.WriteInt32(version);
		writer.WriteInt32(gameMode);
		writer.WriteInt32(scoreCount);
	}
	
	//
	
	ScoreBoard::ScoreBoard()
	{
		Initialize();
	}
	
	void ScoreBoard::Initialize()
	{
		//
	}
	
	void ScoreBoard::Load(int gameMode, Score** out_Scores, int& out_ScoreCount)
	{
		std::string fileName = SAVE_PATH(gameMode);
		
		try
		{
			Validate(fileName.c_str(), VERSION, gameMode);
			
			FileStream stream;
			
			stream.Open(fileName.c_str(), OpenMode_Read);
			
			ScoreFileHeader header;
			
			header.Load(&stream);
			
			*out_Scores = new Score[header.scoreCount];
			out_ScoreCount = header.scoreCount;
			
			for (int i = 0; i < header.scoreCount; ++i)
			{
				(*out_Scores)[i].Load(&stream);
			}
			
			Sort(*out_Scores, out_ScoreCount);
		}
		catch (Exception & e)
		{
			FileStream::Delete(fileName.c_str());
			
			*out_Scores = new Score[1];
			(*out_Scores)[0].Setup(e.what(), 0.0f);
			out_ScoreCount = 1;
		}
	}
	
	void ScoreBoard::Save(int gameMode, Score* score)
	{
		std::string fileName = SAVE_PATH(gameMode);
		
		Validate(fileName.c_str(), VERSION, gameMode);
		
		// update header
		
		ScoreFileHeader header = ReadHeader(fileName.c_str());
		
		header.scoreCount++;
		
		WriteHeader(fileName.c_str(), header);
		
		header = ReadHeader(fileName.c_str());
		
		// write score
		
		FileStream stream;
		
		stream.Open(fileName.c_str(), OpenMode_Append);
		
		score->Save(&stream);
		
		stream.Close();
	}
	
	ScoreFileHeader ScoreBoard::ReadHeader(const char* fileName)
	{
		FileStream stream;
		
		stream.Open(fileName, OpenMode_Read);
		
		ScoreFileHeader header;
		
		header.Load(&stream);
		
		return header;
	}
	
	void ScoreBoard::WriteHeader(const char* fileName, ScoreFileHeader header)
	{
		FileStream stream;
		
		stream.Open(fileName, OpenMode_Append);
		stream.Seek(0, SeekMode_Begin);
		
		header.Save(&stream);
	}
	
	void ScoreBoard::Validate(const char* fileName, int version, int gameMode)
	{
		// check file exists. if not, create a shiny new file
		
		if (!FileStream::Exists(fileName))
		{
			// create file
			
			FileStream stream;
			
			stream.Open(fileName, OpenMode_Write);
			
			stream.Close();
			
			// write header
			
			ScoreFileHeader header;
			
			header.Setup(version, gameMode, 0);
			
			WriteHeader(fileName, header);
		}
		else
		{
			// check version, perform migration/erasure if different
			
			ScoreFileHeader header = ReadHeader(fileName);
			
			if (header.version != version || header.gameMode != gameMode)
			{
				// nope, no migration as of yet
				
				FileStream::Delete(fileName);
				
				Validate(fileName, version, gameMode);
			}
		}
	}
	
	//
	
	static int SortCB(const void* e1, const void* e2)
	{
		const Score* s1 = (Score*)e1;
		const Score* s2 = (Score*)e2;
		
		if (s1->score < s2->score)
			return +1;
		if (s1->score > s2->score)
			return -1;
		
		return 0;
	}
	
	static void Sort(Score* scores, int scoreCount)
	{
		qsort(scores, scoreCount, sizeof(Score), SortCB);
	}
}
