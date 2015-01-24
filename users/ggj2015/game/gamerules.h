#pragma once

#include <string>
#include "gamedefs.h"

class AgendaOption
{
public:
	std::string m_caption;
	std::string m_text;
	bool m_isAttack;

	struct Cost
	{
		Cost()
		{
			memset(this, 0, sizeof(*this));
		}

		int food;
		int wealth;
		int tech;
	} m_cost;

	AgendaOption()
		: m_isAttack(false)
	{
	}

	void parse(const std::string & line);
};

class AgendaEffect
{
public:
	// agenda effect gets executed when the goal is reached / not reached?
	enum OnResult
	{
		OnResult_Success,
		OnResult_Failure
	};

	// effect will affect everyone, the target only, or everyone except the target
	enum Target
	{
		Target_Everyone,
		Target_TargetOnly,
		Target_NonTarget,
		Target_Race
	};

	// any special effects to apply, like multi-round modifiers
	enum SpecialEffect
	{
		SpecialEffect_None,
		SpecialEffect_IncomeModifier
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

	void load(const std::string & text);
	void apply(bool success, int target);
};

class Agenda
{
public:
	Agenda()
		: m_numOptions(0)
	{
	}

	std::string m_description;
	AgendaOption m_options[NUM_VOTING_BUTTONS];
	int m_numOptions;
	std::vector<AgendaEffect> m_effects;

	void apply(bool success, int target)
	{
		for (size_t i = 0; i < m_effects.size(); ++i)
		{
			m_effects[i].apply(success, target);
		}
	}
};
