#pragma once

#include "CompiledComposition.h"

namespace Bandits
{
	class Bandit;
	class EntityBandit;
	
	// Bandit resource -> instance converter
	
	class BanditReader
	{
	public:
		Bandit* Read(EntityBandit* entity, VRCC::CompiledComposition* composition, int level);
	};
}
