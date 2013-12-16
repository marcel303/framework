#pragma once

#include "AnimTimer.h"

namespace Game
{
	class UiGrsRank
	{
	public:
		UiGrsRank();
		void Setup();
		
		void Render();
		void Rank_set(int value);
		
	private:
		int m_Rank;
		AnimTimer m_GrsScoreEffectTimer;
	};
}
