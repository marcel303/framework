#include "framework.h"
#include "gamerules.h"
#include "gamestate.h"


void AgendaEffect::load(const std::string & text)
{
	Dictionary d;

	d.parse(text);

	std::string onResult = d.getString("onresult", "");
	m_onResult = OnResult_Always;
	if (onResult == "success" || onResult == "")
		m_onResult = OnResult_Success;
	else if (onResult == "failure")
		m_onResult = OnResult_Failure;
	else
		logError("invalid onresult: %s", onResult.c_str());

	const std::string target = d.getString("target", "");
	if (target == "everyone")
		m_target = Target_Everyone;
	else if (target == "targetonly")
		m_target = Target_TargetOnly;
	else if (target == "self" || target == "")
		m_target = Target_Self;
	else if (target == "randomrace")
		m_target = Target_Random;
	else if (target == "nontarget")
		m_target = Target_NonTarget;
	else if (target == "sabotage")
		m_target = Target_Sabotage;
	else if (target == "race")
		m_target = Target_Race;
	else if (target == "participating")
		m_target = Target_Participating;
	else if (target == "abstainees")
		m_target = Target_NonParticipating;
	else
		logError("invalid target: %s", target.c_str());

	m_targetRace = d.getInt("race", -1);
	Assert(m_target != Target_Race || m_targetRace != -1);

	m_rewards.food = d.getInt("food", 0);
	m_rewards.wealth = d.getInt("wealth", 0);
	m_rewards.tech = d.getInt("tech", 0);

	const std::string specialEffect = d.getString("special", "");
	if (specialEffect == "")
		m_specialEffect = SpecialEffect_None;
	else if (specialEffect == "incomemod")
		m_specialEffect = SpecialEffect_IncomeModifier;
	else if (specialEffect == "kill")
		m_specialEffect = SpecialEffect_Kill;
	else
		logError("invalid special: %s", specialEffect.c_str());

	const std::string specialEffect2 = d.getString("special2", "");
	if (specialEffect2 == "")
		m_specialEffect2 = SpecialEffect_None;
	else if (m_specialEffect == SpecialEffect_Kill && specialEffect2 == "incomemod")
		m_specialEffect2 = SpecialEffect_IncomeModifier;

	m_specialEffectParam[0] = d.getInt("specialf", 0);
	m_specialEffectParam[1] = d.getInt("specialw", 0);
	m_specialEffectParam[2] = d.getInt("specialt", 0);
}

void AgendaEffect::apply(bool success, int playerId, int * targets, int numTargets)
{
	const bool expectSuccessTest = (m_onResult == OnResult_Success) ? true : false;
	const bool passesSuccessTest = (expectSuccessTest == success) || (m_onResult == OnResult_Always);

	if (passesSuccessTest)
	{
		for (int i = 0; i < g_gameState->m_numPlayers; ++i)
		{
			Player & player = g_gameState->m_players[i];

			if (player.m_isDead)
				continue;
			if (player.m_hasAbstained)
				continue;

			bool passesPlayerTest = false;

			if (m_target == Target_Everyone)
				passesPlayerTest = true;
			else if (m_target == Target_Self)
				passesPlayerTest = i == playerId;
			else if (m_target == Target_TargetOnly)
			{
				for (int t = 0; t < numTargets; ++t)
					passesPlayerTest |= i == targets[t];
			}
			else if (m_target == Target_NonTarget)
			{
				passesPlayerTest = true;
				for (int t = 0; t < numTargets; ++t)
					passesPlayerTest &= i != targets[t];
			}
			else if (m_target == Target_Sabotage)
				passesPlayerTest = player.m_hasSabotaged;
			else if (m_target == Target_Race)
				passesPlayerTest = i == m_targetRace;
			else if (m_target == Target_Participating)
				passesPlayerTest = player.m_hasParticipated;
			else if (m_target == Target_NonParticipating)
				passesPlayerTest = !player.m_hasParticipated;

			if (passesPlayerTest)
			{
				logDebug("agenda: player %d gets awarded (%d/%d/%d)", i, m_rewards.food, m_rewards.wealth, m_rewards.tech);

				player.m_resources.food += m_rewards.food;
				player.m_resources.wealth += m_rewards.wealth;
				player.m_resources.tech += m_rewards.tech;

				switch (m_specialEffect)
				{
				case SpecialEffect_None:
					break;
				case SpecialEffect_IncomeModifier:
					break;
					
				case SpecialEffect_Kill:
					logDebug("agenda: player %d gets killed!", i);
					player.m_shouldBeKilled = true;
					if (g_gameState->m_players[playerId].m_goal.m_killTarget == i)
					{
						logDebug("incrementing kill count for player %d!", playerId);
						g_gameState->m_players[playerId].m_killCount++;
					}
					break;
				}
			}
		}

		// handle special effects

		switch (m_specialEffect)
		{
		case SpecialEffect_None:
			break;
			
		case SpecialEffect_IncomeModifier:
			g_gameState->m_perRoundIncome.food += m_specialEffectParam[0];
			g_gameState->m_perRoundIncome.wealth += m_specialEffectParam[1];
			g_gameState->m_perRoundIncome.tech += m_specialEffectParam[2];
			m_specialEffectParam[0] = 0;
			m_specialEffectParam[1] = 0;
			m_specialEffectParam[2] = 0;
			break;
			
		case SpecialEffect_Kill:
			break;
		}

		switch (m_specialEffect2)
		{
		case SpecialEffect_None:
			break;
			
		case SpecialEffect_IncomeModifier:
			g_gameState->m_perRoundIncome.food += m_specialEffectParam[0];
			g_gameState->m_perRoundIncome.wealth += m_specialEffectParam[1];
			g_gameState->m_perRoundIncome.tech += m_specialEffectParam[2];
			m_specialEffectParam[0] = 0;
			m_specialEffectParam[1] = 0;
			m_specialEffectParam[2] = 0;
			break;
			
		case SpecialEffect_Kill:
			break;
		}
	}
}
