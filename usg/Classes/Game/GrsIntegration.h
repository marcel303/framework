#pragma once

#include "GameSettings.h"

/*

GrsId:

	0 : easy
	1 : hard
	4 : custom

	2 : debug easy
	3 : debug hard
	5 : debug custom

*/

namespace Game
{
	int GameToGrsId(Difficulty difficulty);
	void GrsIdToGame(int grsId, Difficulty & out_Difficulty);

#if defined(IPHONEOS)
	// GameCenter API
	const char * GameToGameCenter(Difficulty difficulty);
#endif

#if defined(BBOS)
	// ScoreLoop API
	int GameToScoreLoop(Difficulty difficulty);
#endif
}
