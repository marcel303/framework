#pragma once

#include <vector>
#include "Debugging.h"
#include "gamerules.h"

#define MAX_PLAYERS 10

class Player;
class GameState;

extern GameState * g_gameState;

class Player
{
public:
	bool m_hasVoted;
	int m_voteSelection;

	struct
	{
		int food;
		int wealth;
		int tech;
	} m_resources;

	Player();

	bool vote(int selection, int target = -1);
	void nextRound();
};

class GameState
{
public:
	Player m_players[MAX_PLAYERS];
	int m_numPlayers;

	std::vector<Agenda> m_agendasLoaded;
	std::vector<Agenda> m_agendas;
	Agenda m_currentAgenda;

	GameState();

	void loadAgendas(const std::vector<std::string> & lines);
	void randomizeAgendaDeck();
	Agenda pickAgendaFromDeck();

	void newGame();
	void nextRound(bool applyCurrentAgenda);
};
