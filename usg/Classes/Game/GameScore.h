#pragma once

namespace Game
{
	class GameScore
	{
	public:
		GameScore();
		~GameScore();
		void Initialize();
		
		void AddScore(int value);
		
		inline int Score_get() const
		{
			return m_Score;
		}
		inline void Score_set(int score)
		{
			m_Score = score;
		}
		
	private:
		int m_Score;
	};
}
