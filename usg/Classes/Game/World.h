#pragma once

#include "BossFactory.h"
#include "Entity.h"
#include "EntityBullet.h"
#include "EntityEnemy.h"
#include "EntityPool.h"
#include "Forward.h"
#include "IntermezzoMgr.h"
#include "libgg_forward.h"
#include "ScoreEffect.h"
#include "SectorGrid.h"
#include "TouchDLG.h"
#include "TouchZoomController.h"
#include "WorldGrid.h"
#include "VectorShape.h"

class SpriteGfx;

//

namespace Game
{
	extern class World* g_World;
	extern Vec2F g_Target;

	enum TouchState
	{
		TouchState_None,
		TouchState_Ambiguous,
		TouchState_Zoom,
		TouchState_Menu
	};
	
	class World
	{
	public:
		World();
		~World();
		
		void Initialize();
		void Setup();
		
		void Update(float dt);
		void Update_Logic(float dt);
		void Update_Animation(float dt);
		void UpdateSB(SelectionBuffer *sb);
		void Render_Background();
		void Render_Background_Primary();
		void Render_Primary_Below();
		void Render_Primary();
		void Render_Additive();
		void Render_Indicators();
	private:
		void Render_Indicator(Entity* entity);
		static void HandleRenderIndicator_Dynamic(void* obj, void* arg);
	public:
		void Render_HudInfo();
		void Render_HudPlayer();
		void Render_Intermezzo();
		
		float HudOpacity_get() const;
		
		// --------------------
		// Touch related
		// --------------------
		
		void PlayerController_set(IPlayerController* controller);
		
		static bool HandleTouchBegin(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchMove(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchEnd(void* obj, const TouchInfo& touchInfo);
		
		// --------------------
		// Saving & loading
		// --------------------
		void Save(Archive& a);
		void Load(Archive& a);
		
		// --------------------
		// Spawning and removing
		// --------------------
		
		EntityEnemy* SpawnEnemy(EntityClass type, const Vec2F& pos, EnemySpawnMode spawnMode);
		void RemoveEnemy(EntityEnemy* enemy);
		void RemoveEnemies_ByClass(EntityClass type);
		EntityEnemy* SpawnBadSector(const Vec2F& pos);
		
		void SpawnBullet(const Bullet& bullet);
		void RemoveBullet(Bullet* b);

		void SpawnPowerup(PowerupType type, const Vec2F& pos, PowerupMoveType moveType);
		
		void SpawnBandit(Res* res, int level, int mods);
		void HandleBanditDeath(Entity* bandit);
		
		Vec2F MakeSpawnPoint_OutsideWorld(float offset) const;
		
		void SpawnZoomParticles(Vec2F position, int count);
		
		int AliveEnemiesCount_get() const;
		int AliveMiniCount_get() const;
		int AliveMaxiCount_get() const;
		int AliveMineCount_get() const;
		int AliveClutterCount_get() const;
		bool IgnoreInEnemyCount(EntityClass type) const;
		bool IncludeInClutterCount(EntityClass type) const;
		
		void HandleKill(Entity* entity);
		
		// --------------------
		// Game state
		// --------------------
		static void HandleWaveBegin(void* obj, void* arg);
		static void HandleBossBegin(void* obj, void* arg);
		static void HandleLevelCleared(void* obj, void* arg);
		static void HandleGameOver(void* obj, void* arg);
		void HandleGameEnd();

		// --------------------
		// Logic
		// --------------------
		void IsPaused_set(bool paused);
		bool IsPaused_get() const;
		
		bool m_IsPaused;
		WorldBorder* m_Border;
		
		// --------------------
		// Controller pause
		// --------------------
		void IsControllerPaused_set(bool paused);
		
		bool m_IsControllerPaused;
		float m_ControllerPausedScalar;
		AnimTimer m_ControllerPausedAnim;
		
		// --------------------
		// Animation
		// --------------------
		
		void CameraOverride_set(bool value);
		
		bool m_CameraOverride;
		ScoreEffectMgr m_ScoreEffectMgr;
		float m_HudOpacity;
		
		// --------------------
		// Intermezzos
		// --------------------
		IntermezzoMgr m_IntermezzoMgr;
		
		// --------------------
		// Entities
		// --------------------
		void AddDynamic(Entity* entity);
		
		static void HandleDynamicRemove(void* obj, void* arg);
		
		void ForEachDynamic(CallBack cb);
		void ForEach_InArea(Vec2F min, Vec2F max, CallBack cb);

		EntityPool<EntityEnemy> m_enemies;
		EntityPool<Bullet> m_bullets;
		EntityPool<EntityEnemy> m_badSectors;
	private:
		DynEntityMgr m_dynamics;
	public:
		// --------------------
		// Bosses
		// --------------------
		BossFactory m_Bosses;
		
		// --------------------
		// Player
		// --------------------
		IPlayerController* m_PlayerController;
		EntityPlayer* m_Player;

		// --------------------
		// Bad sector
		// --------------------
		SectorGrid m_SectorGrid;
		TriggerTimerW m_HeartBeatTrigger;
		
		void Update_HeartBeat();
		float HeartBeat_GetInterval();
		
		// --------------------
		// Backdrop
		// --------------------
		Grid_Effect* m_GridEffect;
		
		// --------------------
		// Touch related
		// --------------------
		TouchState m_TouchState;
		Vec2F m_TouchStartLocations[2];
		Vec2F m_TouchStartLocationsW[2];
		Vec2F m_TouchLocations[2];
		Vec2F m_TouchLocationsW[2];
		Vec2F m_TouchFocusW;
		
		int m_ActiveTouches;
		int m_FingerToTouchIndex[MAX_TOUCHES];
		
		TouchZoomController m_TouchZoomController;
		
		// --------------------
		// Acceleration structures
		// --------------------
		WorldGrid m_WorldGrid;
		
	private:
		int m_AliveCount;
		int m_AliveMiniCount;
		int m_AliveMaxiCount;
		int m_AliveMineCount;
		int m_AliveClutterCount;
		
		VectorShape m_DBG_BossShape;
	};
}
