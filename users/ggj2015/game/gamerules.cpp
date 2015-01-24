#include "framework.h"
#include "gamerules.h"
#include "gamestate.h"

void AgendaEffect::load(const std::string & text)
{
	Dictionary d;

	d.parse(text);

	std::string onResult = d.getString("onresult", "");
	if (onResult == "success")
		m_onResult = OnResult_Success;
	else if (onResult == "failure")
		m_onResult = OnResult_Failure;
	else
		logError("invalid onresult");

	const std::string target = d.getString("target", "");
	if (target == "everyone")
		m_target = Target_Everyone;
	else if (target == "targetonly")
		m_target = Target_TargetOnly;
	else if (target == "nontarget")
		m_target = Target_NonTarget;
	else
		logError("invalid target");

	m_targetRace = d.getInt("target_race", -1);
	Assert(m_target != Target_Race || m_targetRace != -1);

	m_rewards.food = d.getInt("food", 0);
	m_rewards.wealth = d.getInt("wealth", 0);
	m_rewards.tech = d.getInt("tech", 0);

	const std::string specialEffect = d.getString("special", "");
	if (specialEffect == "")
		m_specialEffect = SpecialEffect_None;
	if (specialEffect == "incomemod")
		m_specialEffect = SpecialEffect_IncomeModifier;
	else
		logError("invalid special");
	m_specialEffectParam[0] = d.getInt("special1", 0);
	m_specialEffectParam[1] = d.getInt("special2", 0);
	m_specialEffectParam[2] = d.getInt("special3", 0);
}

void AgendaEffect::apply(bool success, int target)
{
	const bool expectSuccessTest = (m_onResult == OnResult_Success) ? true : false;
	const bool passesSuccessTest = (expectSuccessTest == success);

	if (passesSuccessTest)
	{
		for (int i = 0; i < g_gameState->m_numPlayers; ++i)
		{
			Player & player = g_gameState->m_players[i];

			bool passesPlayerTest = false;

			if (m_target == Target_Everyone)
				passesPlayerTest = true;
			if (m_target == Target_TargetOnly)
				passesPlayerTest = i == target;
			if (m_target == Target_NonTarget)
				passesPlayerTest = i != target;
			if (m_target == Target_Race)
				passesPlayerTest = i == m_targetRace;

			if (passesPlayerTest)
			{
				player.m_resources.food += m_rewards.food;
				player.m_resources.wealth += m_rewards.wealth;
				player.m_resources.tech += m_rewards.tech;
			}
		}

		// handle special effects

		switch (m_specialEffect)
		{
		case SpecialEffect_IncomeModifier:
			//if (m_specialEffectParam[0] == 'f')
			//	g_gameState;
			break;
		}
	}
}
