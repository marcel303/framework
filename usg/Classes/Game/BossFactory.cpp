#include "Boss_Magnet.h"
#include "Boss_Spinner.h"
#include "Boss_Snake.h"
#include "BossFactory.h"
#include "World.h"

namespace Game
{
	BossFactory::BossFactory()
	{
		Initialize();
	}

	BossFactory::~BossFactory()
	{
	}
	
	void BossFactory::Initialize()
	{
	}
	
	BossBase* BossFactory::Create(BossType type)
	{
		switch(type)
		{
		case BossType_Magnet:
			return new Boss_Magnet();
		case BossType_Snake:
			return new Boss_Snake();
		case BossType_Spinner:
			return new Boss_Spinner();
				
		default:
#ifndef DEPLOYMENT
			throw ExceptionVA("unknown boss type");
#else
			return new Boss_Spinner();
#endif
		}
	}
	
	BossBase* BossFactory::Spawn(BossType type, int level)
	{
		BossBase* boss = Create(type);
		
		if (boss)
		{
			boss->Initialize();
			boss->IsAlive_set(XTRUE);
			boss->Setup(level);
			
			g_World->AddDynamic(boss);
		}
		
		return boss;
	}
};
