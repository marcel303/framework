#include "BossBase.h"
#include "GameRound.h"
#include "GameState.h"
#include "World.h"

namespace Game
{
	BossBase::BossBase()
	{
	}
	
	BossBase::~BossBase()
	{
	}
	
	void BossBase::Initialize()
	{
		Entity::Initialize();
		
		// entity overrides
		
		Class_set(EntityClass_MiniBoss);
		Flag_Set(EntityFlag_IsMiniBoss);
		
		// mini boss
		
		m_PowerupType = g_GameState->m_GameRound->Classic_GetPowerupType();
	}
	
	void BossBase::HandleDie()
	{
		g_World->SpawnPowerup(m_PowerupType, Position_get(), PowerupMoveType_GetRandom());
		
		g_World->HandleKill(this);
	}
	
	PowerupType m_PowerupType;
}
