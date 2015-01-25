#pragma once

#include <string>
#include "gamedefs.h"

class Resources
{
public:
	Resources()
	{
		memset(this, 0, sizeof(*this));
	}

	Resources operator-() const
	{
		Resources result;
		result.food = -food;
		result.wealth = -wealth;
		result.tech = -tech;
		return result;
	}

	void operator+=(const Resources & other)
	{
		food += other.food;
		wealth += other.wealth;
		tech += other.tech;
	}

	void operator-=(const Resources & other)
	{
		food -= other.food;
		wealth -= other.wealth;
		tech -= other.tech;
	}

	int food;
	int wealth;
	int tech;
};

class AgendaEffect
{
public:
	// agenda effect gets executed when the goal is reached / not reached?
	enum OnResult
	{
		OnResult_Always,
		OnResult_Success,
		OnResult_Failure
	};

	// effect will affect everyone, the target only, or everyone except the target
	enum Target
	{
		Target_Everyone,
		Target_Self,
		Target_TargetOnly,
		Target_NonTarget,
		Target_Sabotage,
		Target_Race,
		Target_Participating // only those players that actually contribute to accomplishing the agenda
	};

	// any special effects to apply, like multi-round modifiers
	enum SpecialEffect
	{
		SpecialEffect_None,
		SpecialEffect_IncomeModifier,
		SpecialEffect_Kill
	};

	OnResult m_onResult;
	Target m_target;
	int m_targetRace;

	// basic resource effects
	struct
	{
		int food;
		int wealth;
		int tech;
	} m_rewards;

	SpecialEffect m_specialEffect;
	int m_specialEffectParam[3];

	AgendaEffect()
	{
		memset(this, 0, sizeof(*this));
	}

	void load(const std::string & text);
	void apply(bool success, int playerId, int * targets, int numTargets);
};

enum Bribe
{
	Bribe_None,
	Bribe_Transfer,
	Bribe_War
};

class AgendaOption
{
public:
	std::string m_caption;
	std::string m_text;
	bool m_isEnabled;
	bool m_isAttack;
	bool m_isSabotage;
	std::string m_bribeType;
	int m_numTargets;

	Resources m_cost;
	Resources m_bribe;

	AgendaEffect m_effect;

	AgendaOption()
		: m_isEnabled(false)
		, m_isAttack(false)
		, m_isSabotage(false)
		, m_numTargets(0)
	{
	}
};

class Agenda
{
public:
	Agenda()
		: m_numOptions(0)
		, m_percentage(0)
		, m_race(0)
	{
	}

	std::string m_title;
	std::string m_description;
	std::string m_requirement;
	AgendaOption m_options[NUM_VOTING_BUTTONS];
	int m_numOptions;
	std::vector<AgendaEffect> m_effects;
	std::string m_type;
	int m_percentage;
	int m_race;

	struct ResourceConditions
	{
		ResourceConditions()
		{
			memset(this, 0, sizeof(*this));
		}

		int food;
		int wealth;
		int tech;
	} m_resourceConditions;

	void apply(bool success, int * targets, int numTargets)
	{
		for (size_t i = 0; i < m_effects.size(); ++i)
		{
			m_effects[i].apply(success, -1, targets, numTargets);
		}
	}
};
