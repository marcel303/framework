#include "Types.h"

namespace Game
{
	enum BOARDMODE
	{
		BM_LOCAL,
		BM_GLOBAL,
	};
	
	class GameHighScore
	{
	public:
		GameHighScore();
		~GameHighScore();
		
		void Reset()
		{
			m_Mode = BM_LOCAL;
			
			for(int i = 0; i < 50; i++)
			{
				m_Scores[i].score = 0;
				m_Scores[i].name = 0;
				m_Scores[i].nationality = 0;
			}
		}
		
		void Update(float dt);
		void Render();
		
		void OnVisible(); //highscore board visible, load scores.
		
		void SwitchLocalGlobal(); //switch score type, load scores.
		
		void InjectScore(int score, void* name, int nationality); //compare scores and possibly insert a new score.
		
	private:
		void GetGlobal();
		void LoadLocal();
		
		struct Score
		{
			int score;
			void* name;
			int nationality;
		} m_Scores[50];
		
		int m_Index;

		BOARDMODE m_Mode;
	};
}
