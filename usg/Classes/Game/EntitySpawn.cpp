#if 0

#include "Calc.h"
#include "EntitySpawn.h"
#include "GameSettings.h"
#include "GameState.h"
#include "UsgResources.h"
#include "Textures.h"
#include "TempRender.h"
#include "World.h"

#define DIST 150

namespace Game
{
	EntitySpawn::EntitySpawn() : Entity()
	{
	}
	
	EntitySpawn::~EntitySpawn()
	{
	}

	void EntitySpawn::Initialize()
	{
		// entity
		
		Entity::Initialize();
		
		// entity overrides
		
		//Class_set(EntityClass_Spawn);
		Layer_set(EntityLayer_Spawn);
		Position_set(WORLD_MID);
		HitPoints_set(100);
		Flag_Set(EntityFlag_IsFriendly);
		
		// spawn
		
		m_State = State_Done;
		
		m_SequenceTimer.Initialize(&g_GameState->m_TimeTracker_World);
	}

	void EntitySpawn::Setup()
	{
		HitPoints_set(0);
	}
	
	void EntitySpawn::Update(float dt)
	{
		Entity::Update(dt);
		
		switch (m_State)
		{
			case State_Done:
				break;
			
			case State_Sequence1:
				if (m_SequenceTimer.ReadTick())
					State_set(State_Sequence2);
				break;
			case State_Sequence2:
				if (m_SequenceTimer.ReadTick())
					State_set(State_Sequence3);
				break;
			case State_Sequence3:
				if (m_SequenceTimer.ReadTick())
					State_set(State_Done);
				break;
		}
	}
	
	void EntitySpawn::UpdateSB(SelectionBuffer* sb)
	{
	}
	
	void EntitySpawn::Render()
	{
		g_GameState->Render(g_GameState->GetShape(Resources::SPAWN_MIDDLE), Position_get(), Rotation_get(), SpriteColors::Black);
		
#if 0
		// todo: render spawn strip
		
		int n = 3;
		int v = n;
		
		if (m_State == State_Sequence1)
			v = n - 1;
		if (m_State == State_Sequence2)
			v = n - 2;
		if (m_State == State_Sequence3)
			v = n - 3;
		
		for (int i = 0; i < n; ++i)
		{
			VectorShape* shape;
			
			if (i >= v)
				shape = m_Shape_SpawnLight_Orange;
			else
				shape = m_Shape_SpawnLight_Blue;
			
			float offset = i * 45.0f + 45.0f;
			
			Vec2F p1 = Position_get() + Vec2F(-offset, 0.0f);
			Vec2F p2 = Position_get() + Vec2F(+offset, 0.0f);
			Vec2F p3 = Position_get() + Vec2F(0.0f, -offset);
			Vec2F p4 = Position_get() + Vec2F(0.0f, +offset);
			
			g_GameState->Render(m_Shape_SpawnTile, p1, 0.0f, SpriteColors::Black);
			g_GameState->Render(m_Shape_SpawnTile, p2, 0.0f, SpriteColors::Black);
			g_GameState->Render(m_Shape_SpawnTile, p3, Calc::mPI2, SpriteColors::Black);
			g_GameState->Render(m_Shape_SpawnTile, p4, Calc::mPI2, SpriteColors::Black);

			g_GameState->Render(shape, p1, 0.0f, SpriteColors::Black);
			g_GameState->Render(shape, p2, 0.0f, SpriteColors::Black);
			g_GameState->Render(shape, p3, 0.0f, SpriteColors::Black);
			g_GameState->Render(shape, p4, 0.0f, SpriteColors::Black);
		}
#endif
	}
	
	void EntitySpawn::BeginSequence(float duration)
	{
		m_SequenceTimer.SetInterval(duration / 3.0f);
		m_SequenceTimer.Start();
		
		State_set(State_Sequence1);
	}

	void EntitySpawn::SpawnEnemy()
	{
		g_World->m_Bosses.Spawn(BossType_Spinner, 1);
		g_World->m_Bosses.Spawn(BossType_Snake, 1);
	}
	
	void EntitySpawn::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type)
	{
		return;
	}
	
	void EntitySpawn::State_set(State state)
	{
		m_State = state;
		
		switch (m_State)
		{
			case State_Done:
				m_SequenceTimer.Stop();
				break;
			
			case State_Sequence1:
//				g_GameState->m_SoundEffects.Play(Resources::SOUND_PLAYER_SPAWN_LOW, SfxFlag_MustFinish);
				break;
			case State_Sequence2:
//				g_GameState->m_SoundEffects.Play(Resources::SOUND_PLAYER_SPAWN_LOW, SfxFlag_MustFinish);
				break;
			case State_Sequence3:
//				g_GameState->m_SoundEffects.Play(Resources::SOUND_PLAYER_SPAWN, SfxFlag_MustFinish);
				break;
		}
	}
}

#endif
