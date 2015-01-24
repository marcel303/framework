#include <algorithm>
#include <string.h>
#include "main.h"
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
	: m_isDead(false)
	, m_hasVoted(false)
	, m_hasAbstained(false)
	, m_hasParticipated(false)
	, m_voteSelection(0)
	, m_numSelectedTargets(0)
{
}

bool Player::vote(int selection, bool abstain)
{
	Assert(!m_hasVoted);
	if (!m_hasVoted)
	{
		m_hasVoted = true;
		m_hasAbstained = abstain;
		m_voteSelection = selection;

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
	m_isDead = false;

	m_resources.food = 5;
	m_resources.wealth = 5;
	m_resources.tech = 5;

	m_oldResources = m_resources;
}

void Player::nextRound(bool isNewGame)
{
	Assert(m_hasVoted || !isNewGame);
	m_hasVoted = false;

	m_hasAbstained = false;
	m_hasSabotaged = false;
	m_hasParticipated = false;

	m_numSelectedTargets = 0;

	m_resources.food++;
	m_resources.wealth++;
	m_resources.tech++;
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

	for (int i = 0; i < 10; ++i)
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
		if (line.empty())
			continue;

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
				if (startsWith(line, "title", substr))
					agenda->m_title = String::Trim(substr);
				else if (startsWith(line, "description", substr))
					agenda->m_description = String::Trim(substr);
				else if (startsWith(line, "requirement", substr))
					agenda->m_requirement = String::Trim(substr);
				else
				{
					Dictionary d;
					d.parse(line);
					agenda->m_type = d.getString("type", "");
					if (agenda->m_type.empty())
						logError("invalid agenda type");
					agenda->m_percentage = d.getInt("percentage", 100);
					agenda->m_race = d.getInt("race", 0);
					agenda->m_resourceConditions.food = d.getInt("food", 0);
					agenda->m_resourceConditions.wealth = d.getInt("wealth", 0);
					agenda->m_resourceConditions.tech = d.getInt("tech", 0);
				}
			}
			else if (mode == Mode_AgendaOptions)
			{
				AgendaOption & option = agenda->m_options[agenda->m_numOptions];

				if (optionCounter == 0)
				{
					Dictionary d;
					d.parse(line);
					option.m_isEnabled = true;
					option.m_isAttack = d.getBool("isattack", false);
					option.m_isSabotage = d.getBool("sabotage", false);
					option.m_isBribe = d.getBool("bribe", false);
					option.m_numTargets = d.getInt("numtargets", 0);
					option.m_cost.food = d.getInt("food", 0);
					option.m_cost.wealth = d.getInt("wealth", 0);
					option.m_cost.tech = d.getInt("tech", 0);
					option.m_bribe.wealth = d.getInt("bribe_wealth", 0);
				}
				if (optionCounter == 1)
					option.m_effect.load(line);
				if (optionCounter == 2)
					option.m_caption = line;
				if (optionCounter == 3)
					option.m_text = line;

				optionCounter = (optionCounter + 1) % 4;

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

	if (g_devMode)
		std::reverse(m_agendas.begin(), m_agendas.end());
	else
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

	// validate target list

	for (int i = 0; i < m_numPlayers; ++i)
	{
		if (m_players[i].m_numSelectedTargets == 0)
			m_players[i].m_targetSelection[m_players[i].m_numSelectedTargets++] = i;
	}

	// apply bribes

	for (int i = 0; i < m_numPlayers; ++i)
	{
		AgendaOption & option = m_currentAgenda.m_options[m_players[i].m_voteSelection];

		if (option.m_isBribe)
		{
			for (int t = 0; t < m_players[i].m_numSelectedTargets; ++t)
			{
				m_players[m_players[i].m_targetSelection[t]].m_resources += option.m_bribe;
			}
		}
	}

	for (int i = 0; i < m_numPlayers; ++i)
	{
		m_players[i].m_oldResources = m_players[i].m_resources;
	}

	if (applyCurrentAgenda)
	{
		// todo : was the agenda successful or not? add custom code per agenda?

		bool success = true;

		if (m_currentAgenda.m_type == "stockpile")
		{
			bool sabotaged = false;

			Resources total;

			for (int i = 0; i < m_numPlayers; ++i)
			{
				if (m_players[i].m_hasSabotaged)
					sabotaged = true;
				else
				{
					total += m_players[i].m_resourcesSpent;
					m_players[i].m_hasParticipated = true;
				}
			}

			success &= (m_currentAgenda.m_resourceConditions.food == 0) || (total.food >= m_currentAgenda.m_resourceConditions.food);
			success &= (m_currentAgenda.m_resourceConditions.wealth == 0) || (total.wealth >= m_currentAgenda.m_resourceConditions.wealth);
			success &= (m_currentAgenda.m_resourceConditions.tech == 0) || (total.tech >= m_currentAgenda.m_resourceConditions.tech);
		}
		else if (m_currentAgenda.m_type == "attack")
		{
		}
		else if (m_currentAgenda.m_type == "majorityvote")
		{
			// check for majority vote

			int numParticipants = 0;

			for (int i = 0; i < m_numPlayers; ++i)
			{
				if (m_players[i].m_voteSelection == 0)
				{
					numParticipants++;
					m_players[i].m_hasParticipated = true;
				}
			}

			success &= numParticipants >= m_numPlayers * m_currentAgenda.m_percentage / 100;
		}
		else if (m_currentAgenda.m_type == "local")
		{
		}
		else
		{
			logError("invalid agenda type: %s", m_currentAgenda.m_type.c_str());
		}

		for (int i = 0; i < m_numPlayers; ++i)
		{
			if (m_players[i].m_isDead)
				continue;

			AgendaOption & option = m_currentAgenda.m_options[m_players[i].m_voteSelection];

			option.m_effect.apply(success, i, m_players[i].m_targetSelection, m_players[i].m_numSelectedTargets);
		}

		m_currentAgenda.apply(success, 0, 0);
	}

	// check for player goal completion

	std::vector<int> winningPlayers;

	for (int i = 0; i < m_numPlayers; ++i)
	{
		if (!m_players[i].m_isDead && m_players[i].m_goal.isComplete(m_players[i]))
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
			m_players[i].nextRound(applyCurrentAgenda);
		}

		m_currentAgenda = pickAgendaFromDeck();
	}
}
