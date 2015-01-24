#include <algorithm>
#include <string.h>
#include "framework.h"
#include "gamerules.h"
#include "gamestate.h"

Player::Player()
{
	memset(this, 0, sizeof(*this));
}

bool Player::vote(int selection, int target)
{
	Assert(!m_hasVoted);
	if (!m_hasVoted)
	{
		m_hasVoted = true;
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

void Player::nextRound()
{
	Assert(m_hasVoted);
	m_hasVoted = false;
}

//

GameState::GameState()
	: m_numPlayers(0)
{
}

void GameState::loadAgendas(const std::vector<std::string> & lines)
{
	enum Mode
	{
		Mode_None,
		Mode_Agenda,
		Mode_AgendaEffects
	};

	Mode mode = Mode_None;
	Agenda * agenda = 0;

	for (size_t i = 0; i < lines.size(); ++i)
	{
		const std::string & line = lines[i];

		if (line == "agenda")
		{
			mode = Mode_Agenda;

			m_agendasLoaded.push_back(Agenda());
			agenda = &m_agendasLoaded.back();
		}
		else if (line == "effects")
		{
			mode = Mode_AgendaEffects;
		}
		else
		{
			if (mode == Mode_Agenda)
			{
				// todo : load meta data
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
	randomizeAgendaDeck();

	nextRound(false);
}

void GameState::nextRound(bool applyCurrentAgenda)
{
	if (applyCurrentAgenda)
	{
		// todo : was the agenda successful or not? add custom code per agenda?

		bool success = true;

		m_currentAgenda.apply(success, 0);
	}

	for (int i = 0; i < m_numPlayers; ++i)
	{
		m_players[i].nextRound();
	}

	m_currentAgenda = pickAgendaFromDeck();
}
