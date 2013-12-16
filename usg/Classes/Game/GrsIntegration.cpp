#include "GrsIntegration.h"

namespace Game
{
	// shared

	int GameToGrsId(Difficulty difficulty)
	{
	#ifdef DEPLOYMENT
		if (difficulty == Difficulty_Easy)
			return 0;
		if (difficulty == Difficulty_Hard)
			return 1;
		if (difficulty == Difficulty_Custom)
			return 4;
	#else
		if (difficulty == Difficulty_Easy)
			return 2;
		if (difficulty == Difficulty_Hard)
			return 3;
		if (difficulty == Difficulty_Custom)
			return 5;
	#endif

		#ifndef DEPLOYMENT
		throw ExceptionVA("unknown game mode");
		#else
		return 0;
		#endif
	}
	
	void GrsIdToGame(int grsId, Difficulty & out_Difficulty)
	{
		switch (grsId)
		{
		#ifdef DEPLOYMENT
			case 0:
				out_Difficulty = Difficulty_Easy;
				break;
			case 1:
				out_Difficulty = Difficulty_Hard;
				break;
			case 4:
				out_Difficulty = Difficulty_Custom;
				break;
		#else
			case 2:
				out_Difficulty = Difficulty_Easy;
				break;
			case 3:
				out_Difficulty = Difficulty_Hard;
				break;
			case 5:
				out_Difficulty  = Difficulty_Custom;
				break;
		#endif
			default:
				#ifndef DEPLOYMENT
				throw ExceptionVA("unknown game mode");
				#else
				out_Difficulty = Difficulty_Easy;
				break;
				#endif
		}
	}
	
#ifdef IPHONEOS

	// GameCenter API

	const char * GameToGameCenter(Difficulty difficulty)
	{
	#ifdef IPAD
		switch (difficulty)
		{
			case Difficulty_Easy:
				return "critwave.ipad.easy";
			case Difficulty_Hard:
				return "critwave.ipad.hard";
			case Difficulty_Custom:
				return "critwave.ipad.custom";
			default:
				#ifndef DEPLOYMENT
				throw ExceptionVA("unknown game mode");
				#else
				return "critwave.ipad.custom";
				#endif
		}
	#else
		switch (difficulty)
		{
			case Difficulty_Easy:
				return "easy";
			case Difficulty_Hard:
				return "hard";
			case Difficulty_Custom:
				return "custom";
			default:
				#ifndef DEPLOYMENT
				throw ExceptionVA("unknown game mode");
				#else
				return "custom";
				#endif
		}
	#endif
	}

#endif

#ifdef BBOS

	// ScoreLoop API

	int GameToScoreLoop(Difficulty difficulty)
	{
		switch (difficulty)
		{
			case Difficulty_Easy:
				return 0;
			case Difficulty_Hard:
				return 1;
			case Difficulty_Custom:
				return 2;
			default:
				#ifndef DEPLOYMENT
				throw ExceptionVA("unknown game mode");
				#else
				return 0;
				#endif
		}
	}

#endif
}
