#pragma once

#include <string>
#include "libgg_forward.h"

namespace Game
{
	class Score
	{
	public:
		void Setup(const std::string& name, float score);
		void Load(Stream* stream);
		void Save(Stream* stream);
		
		std::string name;
		float score;
	};
	
	class ScoreFileHeader
	{
	public:
		void Setup(int version, int gameMode, int scoreCount);
		void Load(Stream* stream);
		void Save(Stream* stream);
		
		int version;
		int gameMode;
		int scoreCount;
	};
	
	class ScoreBoard
	{
	public:
		ScoreBoard();
		void Initialize();
		
		void Load(int gameMode, Score** out_Scores, int& out_ScoreCount);
		void Save(int gameMode, Score* score);
		
	private:
		ScoreFileHeader ReadHeader(const char* fileName);
		void WriteHeader(const char* fileName, ScoreFileHeader header);
		void Validate(const char* fileName, int version, int gameMode);
	};
}
