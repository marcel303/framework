#pragma once

#include "Debugging.h"

#define MAX_PLAYERS 10

class Player;
class GameState;

extern GameState * g_gameState;

class Player
{
public:
	bool m_hasVoted;

	Player();

	void vote(int selection);
	void nextRound();
};

class GameState
{
public:
	Player m_players[MAX_PLAYERS];
	int m_numPlayers;

	GameState();
	void nextRound();
};
