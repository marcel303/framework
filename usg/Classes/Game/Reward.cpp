#include "Debugging.h"
#include "Reward.h"

namespace Game
{
	Reward::Reward()
	{
		m_Type = RewardType_Undefined;
		m_Value = 0;
	}
	
	Reward::Reward(RewardType type)
	{
		m_Type = type;
		
		switch (type)
		{
			case RewardType_ScoreBronze:
				m_Value = 10;
				break;
			case RewardType_ScoreSilver:
				m_Value = 20;
				break;
			case RewardType_ScoreGold:
				m_Value = 50;
				break;
			case RewardType_ScorePlatinum:
				m_Value = 1000;
				break;
			case RewardType_None:
				m_Value = 0;
				break;
				
			default:
				m_Value = 0;
		}
	}
	
	Reward::Reward(RewardType type, int value)
	{
		Assert(type == RewardType_ScoreCustom);
		
		m_Type = type;
		m_Value = value;
	}
	
	RewardType m_Type;
	int m_Value;
}
