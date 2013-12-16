#pragma once

#include "BossBase.h"

namespace Game
{
	class BossFactory
	{
	public:
		BossFactory();
		~BossFactory();
		void Initialize();
		
		BossBase* Create(BossType type);
		BossBase* Spawn(BossType type, int level);
	};
}
