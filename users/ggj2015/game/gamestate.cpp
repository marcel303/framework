#include <algorithm>
#include <string.h>
#include "framework.h"
#include "gamerules.h"
#include "gamestate.h"
#include "StringEx.h"

bool PlayerGoal::isComplete(const Player & player) const
{
	switch (m_type)
	{
	case Type_Resource:
		{
			bool complete = true;
			complete &= player.m_resources.food >= m_requiredFood;
			complete &= player.m_resources.wealth >= m_requiredWealth;
			complete &= player.m_resources.tech >= m_requiredTech;
			return complete;
		}
		break;

	default:
		Assert(false);
		break;
	}

	return false;
}

Player::Player()
	: m_hasVoted(false)
	, m_voteSelection(0)
{
}

bool Player::vote(int selection, int target)
{
	Assert(!m_hasVoted);
	if (!m_hasVoted)
	{
		m_hasVoted = true;
		m_voteSelection = selection;
		m_targetSelection = target;

		// check if everyone has voted

		bool allVoted = true;

		for (int i = 0; i < g_gameState->m_numPlayers; ++i)
			allVoted &= g_gameState->m_players[i].m_hasVoted;

		if (allVoted)
		{
			g_gameState->nextRound(true);
			return true;
		}
	}

	return false;
}

void Player::newGame()
{
	m_resources.food = 5;
	m_resources.wealth = 5;
	m_resources.tech = 5;
}

void Player::nextRound()
{
	Assert(m_hasVoted);
	m_hasVoted = false;
}

//

GameState::GameState()
	: m_numPlayers(0)
{
	setupPlayerGoals();
}

void GameState::setupPlayerGoals()
{
	// fixme : need at least one unique goal per player..

	{
		PlayerGoal goal;
		goal.m_description = "Gain 15 food";
		goal.m_requiredFood = 15;
		m_playerGoals.push_back(goal);
	}

	{
		PlayerGoal goal;
		goal.m_description = "Gain 15 wealth";
		goal.m_requiredWealth = 15;
		m_playerGoals.push_back(goal);
	}

	{
		PlayerGoal goal;
		goal.m_description = "Gain 15 tech";
		goal.m_requiredTech = 15;
		m_playerGoals.push_back(goal);
	}

	{
		PlayerGoal goal;
		goal.m_description = "Gain 10 food, wealth and tech";
		goal.m_requiredFood = 10;
		goal.m_requiredWealth = 10;
		goal.m_requiredTech = 10;
		m_playerGoals.push_back(goal);
	}
}

static bool startsWith(const std::string & str, const char * with, std::string & out)
{
	if (strstr(str.c_str(), with) != str.c_str())
		return false;
	else
	{
		out = str.substr(strlen(with));
		return true;
	}
}

void GameState::loadAgendas(const std::vector<std::string> & lines)
{
	enum Mode
	{
		Mode_None,
		Mode_Agenda,
		Mode_AgendaOptions,
		Mode_AgendaEffects
	};

	Mode mode = Mode_None;
	Agenda * agenda = 0;
	std::string substr;
	int optionCounter = 0;

	for (size_t i = 0; i < lines.size(); ++i)
	{
		const std::string line = String::Trim(lines[i]);

		if (line == "agenda")
		{
			mode = Mode_Agenda;

			m_agendasLoaded.push_back(Agenda());
			agenda = &m_agendasLoaded.back();
		}
		else if (line == "options")
		{
			mode = Mode_AgendaOptions;
		}
		else if (line == "effects")
		{
			mode = Mode_AgendaEffects;
		}
		else
		{
			if (mode == Mode_Agenda)
			{
				if (startsWith(line, "description", substr))
					agenda->m_description = String::Trim(substr);
			}
			else if (mode == Mode_AgendaOptions)
			{
				AgendaOption & option = agenda->m_options[agenda->m_numOptions];

				if (optionCounter == 0)
				{
					Dictionary d;
					d.parse(line);
					option.m_isAttack = d.getBool("isattack", false);
					option.m_cost.food = d.getInt("food", 0);
					option.m_cost.wealth = d.getInt("wealth", 0);
					option.m_cost.tech = d.getInt("tech", 0);
				}
				if (optionCounter == 1)
					option.m_caption = line;
				if (optionCounter == 2)
					option.m_text = line;

				optionCounter = (optionCounter + 1) % 3;

				if (optionCounter == 0)
				{
					agenda->m_numOptions++;
					Assert(agenda->m_numOptions <= 4);
				}
			}
			else if (mode == Mode_AgendaEffects)
			{
				AgendaEffect effect;
			
				effect.load(line);

				agenda->m_effects.push_back(effect);
			}
			else
			{
				logError("error parsing rule set");
			}
		}
	}
}

void GameState::assignPlayerGoals()
{
	std::vector<int> goals;

	for (int i = 0; i < m_playerGoals.size(); ++i)
		goals.push_back(i);

	std::random_shuffle(goals.begin(), goals.end());

	for (int i = 0; i < m_numPlayers; ++i)
	{
		m_players[i].m_goal = m_playerGoals[goals[i]];
	}
}

void GameState::randomizeAgendaDeck()
{
	m_agendas.clear();

	m_agendas = m_agendasLoaded;

	std::random_shuffle(m_agendas.begin(), m_agendas.end());
}

Agenda GameState::pickAgendaFromDeck()
{
	if (m_agendas.empty())
	{
		randomizeAgendaDeck();
	}
	
	Assert(!m_agendas.empty());
	Agenda agenda = m_agendas.back();
	m_agendas.pop_back();

	return agenda;
}

void GameState::newGame()
{
	m_state = State_Playing;

	randomizeAgendaDeck();

	assignPlayerGoals();

	for (int i = 0; i < m_numPlayers; ++i)
	{
		m_players[i].newGame();
	}

	nextRound(false);
}

void GameState::nextRound(bool applyCurrentAgenda)
{
	Assert(m_state == State_Playing);

	if (applyCurrentAgenda)
	{
		// todo : was the agenda successful or not? add custom code per agenda?

		bool success = true;

		m_currentAgenda.apply(success, 0);
	}

	// check for player goal completion

	std::vector<int> winningPlayers;

	for (int i = 0; i < m_numPlayers; ++i)
	{
		if (m_players[i].m_goal.isComplete(m_players[i]))
		{
			winningPlayers.push_back(i);
		}
	}

	if (!winningPlayers.empty())
	{
		m_state = State_GameEnded;
	}
	else
	{
		for (int i = 0; i < m_numPlayers; ++i)
		{
			m_players[i].nextRound();
		}

		m_currentAgenda = pickAgendaFromDeck();
	}
}
