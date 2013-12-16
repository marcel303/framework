#pragma once

#include "ColList.h"
#include "Entity.h"
#include "EntityPowerup.h"
#include "MiniEntityMgr.h"
#include "Types.h"

namespace Game
{
	enum BossType
	{
		BossType_Undefined = -1,
		BossType_Snake,
		BossType_Spinner,
		BossType_Magnet,
		BossType__Count
	};
	
	class BossBase : public Entity
	{
	public:
		BossBase();
		virtual ~BossBase();
		virtual void Initialize();
		
		virtual void Setup(int level) = 0;
		
	protected:
		virtual void HandleDie();
		
		PowerupType m_PowerupType;
	};
}
