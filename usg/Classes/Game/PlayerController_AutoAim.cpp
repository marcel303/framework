#include "Bandit.h"
#include "BanditEntity.h"
#include "Benchmark.h"
#include "PlayerController_AutoAim.h"
#include "World.h"

namespace Game
{
	struct FireAimSearchState
	{
		Entity* entity;
		float bestHueristic;
		Vec2F pos;
	};
	
	static float EvaluateFireAimTarget(const Vec2F& pos, const Entity* target)
	{
		const float sweetDistance = 0.0f;
		
		const Vec2F delta = target->Position_get() - pos;
		
		float hueristic = Calc::Abs(delta.Length_get() - sweetDistance);
		
		// penalize basic enemies already being tracked
		
		if (target->Flag_IsSet(EntityFlag_IsSmallFry) && target->Flag_IsSet(EntityFlag_TrackState))
			hueristic += 200.0f;
		
		return hueristic;
	}
	
	static void EvalFireAim(void* obj, void* arg)
	{
		FireAimSearchState* search = (FireAimSearchState*)obj;
		Entity* entity = (Entity*)arg;
		
		if (entity->IsAlive_get() && !entity->Flag_IsSet(EntityFlag_IsFriendly))
		{
			if (entity->Class_get() == EntityClass_MaxiBoss)
			{
				Bandits::EntityBandit* bandit = (Bandits::EntityBandit*)entity;
				
				bandit->ForEach_Link(CallBack(obj, EvalFireAim));
			}
			else
			{
				float hueristic = EvaluateFireAimTarget(search->pos, entity);
				
				if (!search->entity || hueristic < search->bestHueristic)
				{
					search->entity = entity;
					search->bestHueristic = hueristic;
				}
			}
		}
	}
	
	static CD_TYPE PlayerController_SelectAutoAimTarget(Vec2F position)
	{
		FireAimSearchState search;
		
		search.entity = 0;
		search.bestHueristic = 0.0f;
		search.pos = position;
		
		// iterate through dynamics to find best bandit / boss
		
		g_World->ForEachDynamic(CallBack(&search, EvalFireAim));
		
		// iterate through ships to find best ship
		
		for (int i = 0; i < g_World->m_enemies.PoolSize_get(); ++i)
		{
			EntityEnemy& enemy = g_World->m_enemies[i];
			
			if (enemy.IsAlive_get())
			{
				EvalFireAim(&search, &enemy);
			}
		}
		
		//
		
		if (search.entity)
		{
			search.entity->Flag_Set(EntityFlag_TrackState);
			
			return search.entity->SelectionId_get();
		}
		else
			return 0;
	}

	AutoAimController::AutoAimController()
	{
		//mUpdateTrigger.Start(1.0f);
		mUpdateTrigger.Start(0.2f);
		mTarget = 0;
		mAngleController.Speed_set(Calc::m4PI);
	}

	void AutoAimController::Update(Vec2F position, float dt)
	{
		if (mUpdateTrigger.Read())
		{
			mTarget = PlayerController_SelectAutoAimTarget(position);
			mUpdateTrigger.Start(1.5f);
		}
		
		if (mTarget != 0)
		{
			if (g_EntityInfo[mTarget].alive)
			{
				// update aim
				
				Vec2F delta = g_EntityInfo[mTarget].position - position;
				
				mAngleController.TargetAngle_set(Vec2F::ToAngle(delta));
			}
			else
			{
				if (mTarget != 0)
					mUpdateTrigger.Start(0.0f);
				
				mTarget = 0;
			}
		}
		
		mAngleController.Update(dt);
		
		mDirection = Vec2F::FromAngle(mAngleController.Angle_get());
	}

	Vec2F AutoAimController::Aim_get() const
	{
		return mDirection;
	}

	bool AutoAimController::HasTarget_get() const
	{
		return mTarget != 0;
	}
}
