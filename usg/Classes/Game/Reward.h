#pragma once

namespace Game
{
	enum RewardType
	{
		RewardType_Undefined,
		RewardType_ScoreBronze,
		RewardType_ScoreSilver,
		RewardType_ScoreGold,
		RewardType_ScorePlatinum,
		RewardType_ScoreCustom,
		RewardType_None
	};
	
	class Reward
	{
	public:
		Reward();
		Reward(RewardType type);
		Reward(RewardType type, int value);
		
		RewardType m_Type;
		int m_Value;
	};
}
