#pragma once

#include <vector>
#include "Debugging.h"
#include "gamerules.h"

#define MAX_PLAYERS 10

class Player;
class GameState;

extern GameState * g_gameState;

class PlayerGoal
{
public:
	enum Type
	{
		Type_Resource,
		Type_Kill
	};

	std::string m_description;
	Type m_type;
	int m_requiredFood;
	int m_requiredWealth;
	int m_requiredTech;

	int m_killTarget;

	PlayerGoal()
		: m_type(Type_Resource)
		, m_requiredFood(0)
		, m_requiredWealth(0)
		, m_requiredTech(0)
		, m_killTarget(-1)
	{
		m_description = "kill all puppies";
	}

	bool isComplete(const Player & player) const;
};

class Player
{
public:
	bool m_isDead;
	bool m_wasDead;
	bool m_hasVoted;
	bool m_hasAbstained;
	bool m_hadAbstained;
	bool m_hasSabotaged;
	bool m_hasParticipated;
	int m_voteSelection;
	int m_targetSelection[3];
	int m_numSelectedTargets;
	PlayerGoal m_goal;

	bool m_shouldBeKilled;
	int m_killCount;

	Resources m_resources;
	Resources m_resourcesSpent;
	Resources m_resourcesAtStartOfRound;
	Resources m_resourcesGainedThisRound;

	std::vector<int> m_bribedPlayers;
	std::vector<int> m_bribedPlayersPrev; // fixme : things like this shouldn't have to be duped. but cannot be bothered changing it now
	std::vector<int> m_targetSelectionPrev;

	Player();

	bool vote(int selection, bool hasAbstained);

	void newGame();
	void nextRound(bool isNewGame);
};

class GameState
{
public:
	enum State
	{
		State_None,
		State_Playing,
		State_GameEnded
	};

	State m_state;

	std::vector<int> m_winningPlayers;

	Player m_players[MAX_PLAYERS];
	int m_numPlayers;

	Resources m_perRoundIncome;

	std::vector<PlayerGoal> m_playerGoals;

	std::vector<Agenda> m_agendasLoaded;
	std::vector<Agenda> m_agendas;
	Agenda m_currentAgenda;
	Agenda m_previousAgenda;

	GameState();

	void setupPlayerGoals();

	void loadAgendas(const std::vector<std::string> & lines);
	void assignPlayerGoals();
	void randomizeAgendaDeck();
	Agenda pickAgendaFromDeck();

	void newGame();
	void nextRound(bool applyCurrentAgenda);
};
