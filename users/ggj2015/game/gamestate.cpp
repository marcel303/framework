#include <string.h>
#include "gamestate.h"

Player::Player()
{
	memset(this, 0, sizeof(*this));
}

void Player::vote(int selection)
{
	Assert(!m_hasVoted);
	if (!m_hasVoted)
	{
		m_hasVoted = true;

		// todo : check if everyone has voted

		bool allVoted = true;

		for (int i = 0; i < g_gameState->m_numPlayers; ++i)
			allVoted &= g_gameState->m_players[i].m_hasVoted;

		if (allVoted)
		{
			g_gameState->nextRound();
		}
	}
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

void GameState::nextRound()
{
	for (int i = 0; i < m_numPlayers; ++i)
	{
		m_players[i].nextRound();
	}
}
