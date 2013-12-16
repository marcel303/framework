#include "GameScore.h"
#include "World.h"

namespace Game
{
	GameScore::GameScore()
	{
		Initialize();
	}
		
	GameScore::~GameScore()
	{
	}

	void GameScore::Initialize()
	{
		m_Score = 0;
	}

	void GameScore::AddScore(int value)
	{
		m_Score += value;
	}
}
