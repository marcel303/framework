#include "Archive.h"
#include "Bandit.h"
#include "BanditEntity.h"
#include "Camera.h"
#include "EntityPlayer.h"
#include "GameRound.h"
#include "GameSave.h"
#include "GameSettings.h"
#include "GameState.h"
#include "Grid_Effect.h"
#include "Log.h"
#include "PerfCount.h"
#if defined(WIN32) || defined(LINUX) || defined(MACOS)
#include "PlayerController_Gamepad.h"
#endif
#if defined(IPHONEOS) || defined(BBOS)
#include "PlayerController.h"
#endif
#ifdef PSP
#include "PlayerController_Gamepad_Psp.h"
#endif
#include "SceneMgt.h"
#include "SoundEffect.h"
#include "SoundEffectMgr.h"
#include "SpriteGfx.h"
#include "StringBuilder.h"
#include "TempRender.h"
#include "Textures.h"
#include "TouchDLG.h"
#include "UsgResources.h"
#include "World.h"
#include "WorldBorder.h"

#ifdef DEBUG
#include "Boss_Magnet.h"
#include "Boss_Snake.h"
#include "EntityPowerup.h"
#endif

#define SECTOR_POOL_SIZE (SECTORGRID_SX * SECTORGRID_SY)

namespace Game
{
#if defined(DEBUG) && 0
	static void DBG_SpawnPowerups();
	static void DBG_SpawnMiniBosses();
	static void DBG_BadSectorsGalore();
#endif
	
	World* g_World;
	Vec2F g_Target;
	
	World::World()
	{
		Initialize();
	}

	World::~World()
	{
		// clear dynamics multiple times to prevent enemy drops from littering the field (they get added on die, which is invoked on clear)
		
		m_dynamics.Clear();
		m_dynamics.Clear();
		m_dynamics.Clear();

		m_Player = 0;

		//

		delete m_Border;
		m_Border = 0;
		delete m_GridEffect;
		m_GridEffect = 0;

		PlayerController_set(0);

		g_World = 0;
	}

	void World::Initialize()
	{
		Assert(g_World == 0);

		g_World = this;
		
		// Register for touches
		
		TouchListener listener;
		listener.Setup(this, HandleTouchBegin, HandleTouchEnd, HandleTouchMove);
		g_GameState->m_TouchDLG->Register(USG::TOUCH_PRIO_WORLD, listener);
		
		// Create controller
		
		m_PlayerController = 0;
		
#if defined(IPHONEOS)
		IPlayerController* controller = new PlayerController_DualAnalog();
#elif defined(WIN32)
		IPlayerController* controller = new PlayerController_Gamepad();
#elif defined(LINUX)
		IPlayerController* controller = new PlayerController_Gamepad();
#elif defined(MACOS)
		IPlayerController* controller = new PlayerController_Gamepad();
#elif defined(PSP)
		IPlayerController* controller = new PlayerController_Gamepad_Psp();
#elif defined(BBOS)
		IPlayerController* controller = new PlayerController_DualAnalog();
#else
		#error
#endif
		
		PlayerController_set(controller);

		// Setup backdrop
		
		m_GridEffect = new Grid_Effect();
		m_GridEffect->Setup(WORLD_SX, WORLD_SY, WORLD_SX / 11.0f, WORLD_SY / 7.0f);
//		m_GridEffect->Setup(WORLD_SX, WORLD_SY, WORLD_SX / 21.0f, WORLD_SY / 13.0f);
		m_Border = new WorldBorder();
		m_Border->Initialize();
		
		// Setup acceleration structures
		
		m_WorldGrid.Setup(WORLD_SX, WORLD_SY, WORLD_SX / 11.0f, WORLD_SY / 7.0f);
		
		// Logic
		
		m_IsPaused = true;

		// Controller pause
		
		m_IsControllerPaused = false;
		m_ControllerPausedScalar = 1.0f;
		m_ControllerPausedAnim.Initialize(g_GameState->m_TimeTracker_Global, false);
		
		// Camera
		
		m_TouchZoomController.Setup(1.0f / 3.0f, 1.0f / 0.9f, g_GameState->m_Camera->Position_get(), g_GameState->m_Camera->Zoom_get());
		
		// Entities
		
		m_enemies.Setup(SHIP_POOL_SIZE);
		m_badSectors.Setup(SECTOR_POOL_SIZE);
		m_bullets.Setup(BULLET_POOL_SIZE);
		
		m_Player = 0;
		
		m_Bosses.Initialize();
		
		m_dynamics.OnEntityRemove = CallBack(this, HandleDynamicRemove);
		
		m_AliveCount = 0;
		m_AliveMiniCount = 0;
		m_AliveMaxiCount = 0;
		m_AliveMineCount = 0;
		m_AliveClutterCount = 0;
		
		// Touch
		
		m_TouchState = TouchState_None;
		m_ActiveTouches = 0;
	}
	
	void World::Setup()
	{
		// --------------------
		// Game state
		// --------------------
		
		m_CameraOverride = false;
		
		// --------------------
		// Entities
		// --------------------
		
		for (int i = 0; i < m_enemies.PoolSize_get(); ++i)
			if (m_enemies[i].IsAlive_get())
				RemoveEnemy(&m_enemies[i]);
		
		for (int i = 0; i < m_badSectors.PoolSize_get(); ++i)
		{
			if (m_badSectors[i].IsAlive_get())
			{
				m_badSectors[i].m_IsAllocated = XFALSE;
				m_badSectors[i].IsAlive_set(XFALSE);
				m_badSectors.Free(&m_badSectors[i]);
			}
		}
		
		for (int i = 0; i < m_bullets.PoolSize_get(); ++i)
		{
			if (m_bullets[i].m_IsAllocated)
			{
				if (!m_bullets[i].m_IsDead)
					m_bullets[i].m_IsDead = XTRUE;
				RemoveBullet(&m_bullets[i]);
			}
		}
		
		// clear dynamics multiple times to prevent enemy drops from littering the field (they get added on die, which is invoked on clear)
		
		m_dynamics.Clear();
		m_dynamics.Clear();
		m_dynamics.Clear();

		// --------------------
		// Bosses
		// --------------------
		
		m_AliveCount = 0;
		m_AliveMiniCount = 0;
		m_AliveMaxiCount = 0;
		m_AliveMineCount = 0;
		m_AliveClutterCount = 0;
		
		// --------------------
		// Player
		// --------------------
		
		m_Player = 0;

		m_Player = new EntityPlayer();
		m_Player->Initialize();
		m_Player->IsAlive_set(XTRUE);
		m_Player->Setup(Vec2F(WORLD_SX / 2.0f, WORLD_SY / 2.0f));
		m_Player->OnGameOver = CallBack(this, HandleGameOver);
		
		AddDynamic(m_Player);
		
		m_HeartBeatTrigger.Start(1.0f);
		
		// --------------------
		// Bad sector
		// --------------------
		
		m_SectorGrid.Initialize();
		
		// --------------------
		// Backdrop
		// --------------------
		
		m_GridEffect->Clear();
		
		// TEST CODE
		
#ifdef DEBUG
//		DBG_SpawnPowerups();
		//DBG_SpawnMiniBosses();
//		DBG_BadSectorsGalore();
#endif
	}
	
	void World::Update(float dt)
	{
#if 1
		if (m_TouchZoomController.ActiveZoomTarget_get() == ZoomTarget_ZoomedOut)
//			dt *= 0.25f;
//			dt *= 0.05f;
			dt *= 0.15f;
#endif
		dt *= m_Player->ShockTimeDilution_get();
		
		bool wait = 
			(g_GameState->m_GameRound->GameModeIsClassic()) &&
			g_GameState->m_GameRound->Classic_RoundState_get() != RoundState_LevelCleared &&
			m_Player->IsPlaying_get() &&
			m_PlayerController->IsIdle_get();
		
		IsControllerPaused_set(wait);
		
		if (m_IsControllerPaused)
		{
			m_ControllerPausedScalar *= powf(0.01f, dt);
			
			#if SCREENSHOT_MODE == 0
			dt *= m_ControllerPausedScalar;
			#else
			dt = 0.0f;
			#endif
		}
		
		{
			float dtAnim = 1.0f / 60.0f;
			float opacity = g_GameState->m_GameSettings->m_HudOpacity;
			
			if (g_GameState->m_GameSettings->m_ControllerType == ControllerType_DualAnalog)
			{
				if (g_World->m_PlayerController->IsIdle_get() == false)
					opacity *= 0.4f;
			}
			
			if (m_HudOpacity != opacity)
			{
				float delta = opacity - m_HudOpacity;
				float dir = Calc::Sign(delta);
				delta = Calc::Abs(delta);		
				float speed = 4.0f;
				float correction = speed * dtAnim;
				if (correction > delta)
					correction = delta;
				m_HudOpacity = Calc::Saturate(m_HudOpacity + dir * correction);
			}
			
			//LOG_INF("opac: %g", m_HudOpacity);
		}
		
		if (!IsPaused_get())
		{
			Update_Logic(dt);
		}
		
#if 0
#if SCREENSHOT_MODE == 1
		if (!m_IsControllerPaused)
		{
			Update_Animation(dt);
		}
#else
		Update_Animation(dt);
#endif
#endif
		
		Update_HeartBeat();
		
		m_IntermezzoMgr.Update(dt);
	}
	
	void World::Update_Logic(float dt)
	{
		// increment time
		
		g_GameState->m_TimeTracker_World->Increment(dt);
		
		// update game logic
		
		UsingBegin(PerfTimer timer(PC_UPDATE_LOGIC))
		{
			// update bad sector
			
			for (int i = 0; i < SECTOR_POOL_SIZE; i++)
			{
				if (!m_badSectors[i].IsAlive_get())
					continue;
				
				m_badSectors[i].Update(dt);
			}
				
			// update entities
			
			Grid_Effect* gridEffect = m_GridEffect;

			for (int i = 0; i < SHIP_POOL_SIZE; i++)
			{
				if (!m_enemies[i].IsAlive_get())
					continue;
				
				Vec2F oldPosition = m_enemies[i].Position_get();
				
				m_enemies[i].Update(dt);
				
				Vec2F newPosition = m_enemies[i].Position_get();
				
				m_WorldGrid.Update(oldPosition, newPosition, &m_enemies[i]);
				gridEffect->ObjectUpdate(oldPosition, newPosition);
			}
			
			m_dynamics.Update(dt);

			for (int i = 0; i < BULLET_POOL_SIZE; i++)
			{
				if (!m_bullets[i].m_IsAllocated)
					continue;
				
				Vec2F oldPosition = m_bullets[i].m_Pos;
				
				m_bullets[i].Update(dt);
				
				Vec2F newPosition = m_bullets[i].m_Pos;
				
				gridEffect->ObjectUpdate(oldPosition, newPosition);
			}
		}
		UsingEnd()
		
		UsingBegin(PerfTimer timer(PC_UPDATE_POST))
		{
			// clear up dead objects
			
			for (int i = 0; i < SHIP_POOL_SIZE; ++i)
			{
				if (!m_enemies[i].IsAlive_get())
					continue;
				
				if (!(m_enemies[i].m_Flags & EntityFlag_DidDie))
					continue;
				
				m_enemies[i].HandleDie();
				
				RemoveEnemy(&m_enemies[i]);
			}
			
			for (int i = 0; i < BULLET_POOL_SIZE; ++i)
			{
				if (!m_bullets[i].m_IsAllocated)
					continue;
				
				if (!m_bullets[i].m_IsDead)
					continue;
				
				RemoveBullet(&m_bullets[i]);
			}
			
			g_SceneMgt.PostUpdate();
			
			m_dynamics.PostUpdate();
		}
		UsingEnd()
		
		g_Target = m_Player->Position_get();
	}
	
	class MaxiScrollInfo
	{
	public:
		MaxiScrollInfo()
		{
			bossCount = 0;
			laserCount = 0;
		}
		
		Vec2F dir;
		Vec2F delta;
		int bossCount;
		int laserCount;
	};
	
	static void MaxiDirSum(void* obj, void* arg)
	{
		MaxiScrollInfo* info = (MaxiScrollInfo*)obj;
		Entity* entity = (Entity*)arg;
		
		if (entity->Class_get() != EntityClass_MaxiBoss)
			return;
		
		Bandits::EntityBandit* bandit = (Bandits::EntityBandit*)entity;

		info->bossCount++;
		
		if (bandit->IsBeamActive_get())
		{
			Vec2F delta = entity->Position_get() - g_World->m_Player->Position_get();
			Vec2F dir = delta.Normal();
			
			info->dir += dir;
			info->delta += delta;
			info->laserCount++;
		}
	}
	
	class MaxiDistanceQuery
	{
	public:
		MaxiDistanceQuery(Vec2F position)
		{
			mPosition = position;
			mMinDistance = -1.0f;
		}
		
		Vec2F mPosition;
		float mMinDistance;
	};
	
	static void HandleMaxiDistanceQuery(void* obj, void* arg)
	{
		MaxiDistanceQuery* query = (MaxiDistanceQuery*)obj;
		Entity* entity = (Entity*)arg;
		
		if (entity->Class_get() == EntityClass_MaxiBoss)
		{
			Vec2F delta = entity->Position_get() - query->mPosition;
			float distance = delta.Length_get();
			if (distance < query->mMinDistance || query->mMinDistance < 0.0f)
				query->mMinDistance = distance;
		}
	}
	
	void World::Update_Animation(float dt)
	{
		// update touch zoom controller and camera
		
		if (m_AliveMaxiCount > 0)
		{
			MaxiDistanceQuery query(m_Player->Position_get());
			m_dynamics.ForEach(CallBack(&query, HandleMaxiDistanceQuery));
			float distance1 = 100.0f;
			float distance2 = 300.0f;
			float zoom1 = 0.8f;
			float zoom2 = 1.0f;
			float zoom = zoom1 + (zoom2 - zoom1) * (query.mMinDistance - distance1) / (distance2 - distance1);
			zoom = Calc::Mid(zoom, zoom1, zoom2);
#if SCREENSHOT_MODE == 1
#pragma message("screen shot mode: zoom = 1.0")
			zoom = 1.0f;
#endif
			m_TouchZoomController.UpdateTarget(ZoomTarget_Player, zoom, m_Player->Position_get());
		}
		else
		{
			m_TouchZoomController.UpdateTarget(ZoomTarget_Player, 1.0f, m_Player->Position_get());
		}
		
		m_TouchZoomController.Update(dt);
		
		if (!m_CameraOverride)
		{
			const ZoomInfo& zi = m_TouchZoomController.ZoomInfo_get();
			
			g_GameState->m_Camera->Zoom(zi.zoom);
			g_GameState->m_Camera->Focus(zi.position);
		}
		
		MaxiScrollInfo info;
			
		ForEachDynamic(CallBack(&info, MaxiDirSum));
		
		m_GridEffect->BaseMultiplier_set(1.0f / (1.0f + info.laserCount));
		
		if (g_GameState->m_GameRound->Classic_RoundState_get() == RoundState_PlayMaxiBoss)
		{
			if (info.laserCount > 0)
			{
				info.dir.Normalize();
				
//				float scrollSpeed = 50.0f;
				float scrollSpeed = -400.0f / (1.0f + 10.0f * info.delta.Length_get() / WORLD_SY);
				
				Vec2F scrollScale(1.0f / WORLD_SX, 1.0f / WORLD_SY);
				
				m_GridEffect->Scroll((info.dir * scrollSpeed * dt) ^ scrollScale);
			}
		}
		
		m_GridEffect->Update(dt);
		
		m_Border->Update(dt);
		
		m_ScoreEffectMgr.Update(dt);
		
//		m_DebrisMgr.Update(dt);
	}
		
	void World::UpdateSB(SelectionBuffer *sb)
	{
		m_dynamics.UpdateSB(sb);
		
		g_GameState->UpdateSB(&m_DBG_BossShape, 0.0f, 0.0f, 0.0f, 0);
		
		for (int i = 0; i < SHIP_POOL_SIZE; ++i)
		{
			if (!m_enemies[i].IsAlive_get())
				continue;
			
			m_enemies[i].UpdateSB(sb);
		}
	}

	void World::Render_Background()
	{
		if (g_GameState->m_GameRound->GameModeIsInvaders())
		{
			float uv1[2] = { 0.0f, 0.0f };
			float uv2[2] = { 1.0f, 1.0f };
			RenderRect(Vec2F(0.0f, 0.0f), Vec2F(float(VIEW_SX), float(VIEW_SY)), SpriteColors::White, uv1, uv2);
		}
		else if (false)
		{
			float uv1[2] = { 0.0f, 0.0f };
			float uv2[2] = { 1.0f, 1.0f };
			RenderRect(Vec2F(0.0f, 0.0f), Vec2F(float(WORLD_SX), float(WORLD_SY)), SpriteColors::White, uv1, uv2);
		}
		else
		{
			m_GridEffect->Render();
		}
	}
	
	void World::Render_Background_Primary()
	{
		float border = 300.0f;
		RenderRect(Vec2F(-border, -border), Vec2F(WORLD_SX + border * 2.0f, border), SpriteColor_Make(0, 0, 0, 255), g_GameState->GetTexture(Textures::COLOR_WHITE));
		RenderRect(Vec2F(-border, WORLD_SY), Vec2F(WORLD_SX + border * 2.0f, border), SpriteColor_Make(0, 0, 0, 255), g_GameState->GetTexture(Textures::COLOR_WHITE));
		RenderRect(Vec2F(-border, 0.0f), Vec2F(border, WORLD_SY), SpriteColor_Make(0, 0, 0, 255), g_GameState->GetTexture(Textures::COLOR_WHITE));
		RenderRect(Vec2F(WORLD_SX, 0.0f), Vec2F(border, WORLD_SY), SpriteColor_Make(0, 0, 0, 255), g_GameState->GetTexture(Textures::COLOR_WHITE));
		
		Vec2F pos = Vec2F(0.0f, WORLD_SY + 20.0f);
		Vec2F size = Vec2F(WORLD_SX, 25.0f);
				
		//if (g_GameState->m_Camera.m_Area.Max_get()[1] >= pos[1])
		if (true)
		{
			RenderRect(pos, size, SpriteColor_Make(25, 25, 25, 255), g_GameState->GetTexture(Textures::COLOR_WHITE));
			
			if (g_GameState->m_GameRound->GameModeIsClassic())
			{
				StringBuilder<128> sb;
				sb.AppendFormat("level %d | wave %d/%d | lives: %d | kills: %05d | bullets: %05d",
				   g_GameState->m_GameRound->Classic_Level_get(),
				   g_GameState->m_GameRound->Classic_Wave_get() + 1,
				   g_GameState->m_GameRound->Classic_WaveCount_get(),
				   g_World->m_Player->Lives_get(),
				   g_World->m_Player->m_Stat_KillCount,
				   g_World->m_Player->m_Stat_BulletCount);
				RenderText(pos + Vec2F(20.0f, 0.0f), size, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(0.5f, 0.5f, 0.5f, 1.0f), TextAlignment_Left, TextAlignment_Center, true, sb.ToString());
			}
			
			RenderText(pos, size, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(0.5f, 0.5f, 0.5f, 1.0f), TextAlignment_Right, TextAlignment_Center, true,
			   "(C) 2012 Grannies-Games.Com");
		}
	}
	
	void World::Render_Primary_Below()
	{
	}
	
	void World::Render_Primary()
	{
		if (g_GameState->m_GameRound->GameModeTest(GMF_Classic | GMF_IntroScreen))
		{
			m_Border->Render();
		}
		
		g_GameState->m_SpriteGfx->Flush();
		
		m_dynamics.Render_Below();
		
		for (int i = 0; i < SECTOR_POOL_SIZE; i++)
			if (m_badSectors[i].IsAlive_get())
				m_badSectors[i].Render();
			
		for (int i = 0; i < SHIP_POOL_SIZE; i++)
			if (m_enemies[i].IsAlive_get())
				m_enemies[i].Render();
		
		m_dynamics.Render();
		
		for (int i = 0; i < BULLET_POOL_SIZE; i++)
			if (m_bullets[i].m_IsAllocated)
				m_bullets[i].Render();
		
		m_ScoreEffectMgr.Render();
	}

	void World::Render_Additive()
	{
		m_dynamics.Render_Additive();
		
		for (int i = 0; i < BULLET_POOL_SIZE; ++i)
			if (m_bullets[i].m_IsAllocated)
				m_bullets[i].Render_Additive();
	}
	
	void World::Render_Indicator(Entity* entity)
	{
		const VectorShape* shape = 0;
		
		if (entity->Flag_IsSet(EntityFlag_IsMaxiBoss))
		{
			shape = g_GameState->GetShape(Resources::INDICATOR_BANDIT2);
		}
		else if (entity->Flag_IsSet(EntityFlag_IsMiniBoss))
		{
			shape = g_GameState->GetShape(Resources::INDICATOR_BANDIT2);
		}
		else if (entity->Flag_IsSet(EntityFlag_IsMiniBossSegment))
		{
			// nop: don't render indicators for individual segments
		}
		else if (entity->Class_get() == EntityClass_Powerup)
		{
			shape = g_GameState->GetShape(Resources::INDICATOR_POWERUP);
		}
		else if (entity->Class_get() == EntityClass_Mine)
		{
			shape = g_GameState->GetShape(Resources::INDICATOR_ENEMY_MINE);
		}
		else
		{
			shape = g_GameState->GetShape(Resources::INDICATOR_ENEMY);
		}
		
		if (!shape)
			return;
		
		// decide if indicator should be draw (enemy off screen)
		
		Vec2F pos2cam = entity->Position_get() - g_GameState->m_Camera->Position_get();
		
		if (pos2cam[0] < -VIEW_SX/2 || pos2cam[0] > +VIEW_SX/2 || pos2cam[1] < -VIEW_SY/2 || pos2cam[1] > +VIEW_SY/2)
		{
			pos2cam.Normalize();
			
			float angle = Vec2F::ToAngle(pos2cam);
			
			// todo: intersect pos2cam with camera rect
			
			const RectF& area = g_GameState->m_Camera->m_Area;
			
			float intersectT;
			Vec2F pos = Intersect_Rect(area, g_GameState->m_Camera->Position_get(), pos2cam, intersectT);
			
			float pos2ship = (entity->Position_get() - pos).Length_get();
			
			float size = (shape->m_Shape.m_BoundingBox.m_Max[0] - shape->m_Shape.m_BoundingBox.m_Min[0]) / 2.0f + 2.0f;
			
			pos -= pos2cam * size;
			
			// calculate angle and position and draw indicator
		
			int v = Calc::Clamp((int)(pos2ship * 4.0f), 0, 255);
			
#if 0
			SpriteColor color;
			
			if (!entity->Flag_IsSet(EntityFlag_IsMiniBoss))
				color = SpriteColor_Make(v, v >> 1, v >> 4, 255);
			else
				color = SpriteColor_Make(v, v, v, 255);
#else
//			SpriteColor color = SpriteColor_Make(0, 0, 0, v);
			SpriteColor color = SpriteColor_Make(255, 255, 255, v);
#endif
			
			if (/*entity->Flag_IsSet(EntityFlag_IsMiniBoss) || entity->Flag_IsSet(EntityFlag_IsMaxiBoss) || */entity->Flag_IsSet(EntityFlag_IsPowerup))
				angle = 0.0f;
				
			g_GameState->Render(shape, pos, angle, color);
		}
	}
	
	void World::HandleRenderIndicator_Dynamic(void* obj, void* arg)
	{
		World* self = (World*)obj;
		Entity* entity = (Entity*)arg;
		
		self->Render_Indicator(entity);
	}
	
	void World::Render_Indicators()
	{
		if (m_TouchZoomController.ActiveZoomTarget_get() != ZoomTarget_Player)
			return;
		
		// render enemy ship indicators
		
		for (int i = 0; i < SHIP_POOL_SIZE; i++)
		{
			if (m_enemies[i].IsAlive_get())
			{
				Render_Indicator(&m_enemies[i]);
			}
		}
		
		// render indicators for dynamics
		
		m_dynamics.ForEach(CallBack(this, HandleRenderIndicator_Dynamic));
		
		// todo: render powerup indicators
	}
	
	void World::Render_HudInfo()
	{
		m_Player->Render_HUD();
		
		if (m_ControllerPausedAnim.IsRunning_get())
		{
			#if SCREENSHOT_MODE == 0
			RenderText(Vec2F(VIEW_SX/2.0f, VIEW_SY - 140.0f), Vec2F(0.0f, 0.0f), g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, m_ControllerPausedAnim.Progress_get()), TextAlignment_Center, TextAlignment_Top, true, "AUTO-PAUSE");
			RenderText(Vec2F(VIEW_SX/2.0f, VIEW_SY - 110.0f), Vec2F(0.0f, 0.0f), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColor_MakeF(1.0f, 1.0f, 1.0f, m_ControllerPausedAnim.Progress_get()), TextAlignment_Center, TextAlignment_Top, true, "touch to continue");
			#endif
		}
		
		if (m_TouchZoomController.ActiveZoomTarget_get() == ZoomTarget_ZoomedOut)
		{
			// render mine indicators

			for (int i = 0; i < SHIP_POOL_SIZE; ++i)
			{
				EntityEnemy& enemy = m_enemies[i];
				
				if (!enemy.IsAlive_get())
					continue;
				if (enemy.Class_get() != EntityClass_Mine)
					continue;
				
				Vec2F pos = g_GameState->m_Camera->WorldToView(enemy.Position_get());
				
				g_GameState->Render(g_GameState->GetShape(Resources::INDICATOR_MINE_SYMBOL), pos, 0.0f, SpriteColors::White);
				RenderRect(pos + Vec2F(-15.0f, 18.0f), Vec2F(30.0f, 8.0f), 0.0f, 0.0f, 0.0f, 1.0f, g_GameState->GetTexture(Textures::COLOR_WHITE));
				RenderRect(pos + Vec2F(-13.0f, 20.0f), Vec2F(26.0f, 4.0f), 72 / 255.0f, 5 / 255.0f, 111 / 255.0f, 1.0f, g_GameState->GetTexture(Textures::COLOR_WHITE));
				RenderRect(pos + Vec2F(-13.0f, 20.0f), Vec2F(26.0f, 4.0f) ^ Vec2F(enemy.MineProgress_get(), 1.0f), 255 / 255.0f, 234 / 255.0f, 0.0f, 1.0f, g_GameState->GetTexture(Textures::COLOR_WHITE));
			}
		}
	}
	
	void World::Render_HudPlayer()
	{
		m_PlayerController->Render();
	}
	
	void World::Render_Intermezzo()
	{
		m_IntermezzoMgr.Render_Overlay();
	}
	
	float World::HudOpacity_get() const
	{
		return m_HudOpacity;
	}
		
	//

	void World::PlayerController_set(IPlayerController* controller)
	{
		delete m_PlayerController;
		m_PlayerController = 0;
		
		//

		m_PlayerController = controller;
	}
	
	bool World::HandleTouchBegin(void* obj, const TouchInfo& touchInfo)
	{
		World* world = (World*)obj;
		
		if (world->m_ActiveTouches >= 2)
			return false;
		
		world->m_FingerToTouchIndex[touchInfo.m_FingerIndex] = world->m_ActiveTouches;
		
		int touchIndex = world->m_FingerToTouchIndex[touchInfo.m_FingerIndex];
		
		//LOG(LogLevel_Debug, "Recevied touch with id: %d", touchInfo->m_FingerIndex);

		world->m_TouchStartLocations[touchIndex] = touchInfo.m_LocationView;
		world->m_TouchStartLocationsW[touchIndex] = touchInfo.m_Location;
		world->m_TouchLocations[touchIndex] = touchInfo.m_LocationView;
		world->m_TouchLocationsW[touchIndex] = touchInfo.m_Location;

//		if (world->m_TouchZoomController.IsActive(ZoomTarget_Manual) && world->m_TouchZoomController.ManualZoomState_get() == ManualZoomState_ZoomedOut)
		if (world->m_TouchZoomController.IsActive(ZoomTarget_ZoomedOut))
		{
			// zoomed out, user touched screen - zoom back in
			
//			world->m_TouchZoomController.End(false);
			world->m_TouchZoomController.Deactivate(ZoomTarget_ZoomedOut);
		}
		else
		{
			switch (world->m_TouchState)
			{
				case TouchState_None:
				{
					if (world->m_ActiveTouches == 0)
					{
						if (true)
						{
							// go from none -> ambiguous
							
							world->m_TouchState = TouchState_Ambiguous;
							
							break;
						}
					}
				}
				break;
					
				case TouchState_Ambiguous:
				{
					if (world->m_ActiveTouches == 1)
					{
						// go from ambiguous -> zoom
						
						world->m_TouchState = TouchState_Zoom;
						
						world->m_TouchZoomController.Begin(world->m_TouchLocations[0], world->m_TouchLocations[1], g_GameState->m_Camera->Zoom_get(), g_GameState->m_Camera->Position_get());
						
						world->m_TouchFocusW = (world->m_TouchLocationsW[0] + world->m_TouchLocationsW[1]) / 2.0f;
					}
				}
				break;
					
			default:
				break;
			}
		}
			
		world->m_ActiveTouches++;
		
		LOG(LogLevel_Debug, "active touch count: %d", world->m_ActiveTouches);
		
		return true;
	}
		
	bool World::HandleTouchMove(void* obj, const TouchInfo& touchInfo)
	{
		World* world = (World*)obj;
		
		int touchIndex = world->m_FingerToTouchIndex[touchInfo.m_FingerIndex];
		
		world->m_TouchLocations[touchIndex] = touchInfo.m_LocationView;
		world->m_TouchLocationsW[touchIndex] = touchInfo.m_Location;
		
		switch (world->m_TouchState)
		{
		case TouchState_None:
			break;

		case TouchState_Ambiguous:
			{
				Vec2F delta = world->m_TouchLocations[0] - world->m_TouchStartLocations[0];
				
				if (delta.Length_get() > 10.0f)
				{
					world->m_TouchState = TouchState_Zoom;
					
					world->m_TouchZoomController.Begin(world->m_TouchLocations[0], g_GameState->m_Camera->Zoom_get(), g_GameState->m_Camera->Position_get());
				}
			}
			break;

		case TouchState_Zoom:
			{
				world->m_TouchZoomController.Update(touchIndex, world->m_TouchLocations[touchIndex]);
				
				#if SCREENSHOT_MODE == 0
				world->SpawnZoomParticles(Vec2F(touchInfo.m_Location), 10);
				#endif
				
				break;
			}

		case TouchState_Menu:
			{
			}
			break;

		default:
			break;
		}
		
		return true;
	}

	bool World::HandleTouchEnd(void* obj, const TouchInfo& touchInfo)
	{
		World* world = (World*)obj;

		LOG(LogLevel_Debug, "Recevied EndTouch with id: %d", touchInfo.m_FingerIndex);
		
		LOG(LogLevel_Debug, "State: %d", world->m_TouchState);
		
		switch (world->m_TouchState)
		{
		case TouchState_None:
			break;
				
		case TouchState_Ambiguous:
			{
				if (world->m_ActiveTouches == 1)
				{
					world->m_TouchState = TouchState_None;
					
					world->m_TouchZoomController.End(true);
				}
			}
			break;

		case TouchState_Zoom:
			{
				world->m_TouchZoomController.End(true);
					
				world->m_TouchState = TouchState_None;
			}
			break;
				
		case TouchState_Menu:
			{
			}	
			break;
				
		default:
			break;
		}
		
		world->m_ActiveTouches--;
		
		LOG(LogLevel_Debug, "active touch count: %d", world->m_ActiveTouches);
		
		if (world->m_ActiveTouches == 0)
		{
			Assert(world->m_TouchState == TouchState_None); 
		}
		
		return true;
	}
	
	// --------------------
	// Saving & loading
	// --------------------
	
	void World::Save(Archive& a)
	{
		a.WriteSection("sectors");
		m_SectorGrid.Save(a);
		a.WriteSectionEnd();
		a.WriteSection("mines");
		for (int i = 0; i < m_enemies.PoolSize_get(); ++i)
		{
			EntityEnemy& enemy = m_enemies[i];
			
			if (!enemy.IsAlive_get())
				continue;
			if (enemy.Class_get() != EntityClass_Mine)
				continue;
			
			a.WriteSection("mine");
			enemy.Mine_Save(a);
			a.WriteSectionEnd();
			
		}
		a.WriteSectionEnd();
	}
	
	void World::Load(Archive& a)
	{
		while (a.NextSection())
		{
			if (a.IsSection("sectors"))
			{
				m_SectorGrid.Load(a);
			}
			else if (a.IsSection("mines"))
			{
				while (a.NextSection())
				{
					if (a.IsSection("mine"))
					{
#if 1
						while (a.NextValue())
						{
						}
#else
						EntityEnemy* enemy = SpawnEnemy(EntityClass_Mine, Vec2F(), EnemySpawnMode_Instant);
						
						if (!enemy)
							continue;
						
						enemy->Mine_Load(a);
#endif
					}
					else
					{
#ifndef DEPLOYMENT
						throw ExceptionVA("unknown section: %s", a.GetSection());
#endif
					}
				}
			}
			else
			{
#ifndef DEPLOYMENT
				throw ExceptionVA("unknown section: %s", a.GetSection());
#endif
			}
		}
	}

	// --------------------
	// Spawning and removing
	// --------------------
	
	void World::SpawnBullet(const Bullet& _bullet)
	{
		Bullet* bullet = m_bullets.Allocate();
		
		// FIXME: *MUST* allocate bullet. destroy an allocated bullet if must
		
		if (!bullet)
		{
			return;
		}
		
#ifdef DEBUG
		Assert(bullet->m_IsDead == XTRUE);
		Assert(bullet->m_IsAllocated == XFALSE);
#endif

		*bullet = _bullet;
		
		bullet->m_IsAllocated = XTRUE;
		bullet->m_IsDead = XFALSE;
		
		bullet->PlaySound(BulletSound_Fire);
		
		m_GridEffect->ObjectAdd(bullet->m_Pos);
	}

	void World::RemoveBullet(Bullet* bullet)
	{
#ifdef DEBUG
		Assert(bullet->m_IsDead == XTRUE);
		Assert(bullet->m_IsAllocated == XTRUE);
#endif

		m_GridEffect->ObjectRemove(bullet->m_Pos);

		bullet->m_IsAllocated = XFALSE;
		
		m_bullets.Free(bullet);
	}

	EntityEnemy* World::SpawnEnemy(EntityClass type, const Vec2F& pos, EnemySpawnMode spawnMode)
	{
		EntityEnemy* enemy = m_enemies.Allocate();
		
		if (!enemy)
		{
			return 0;
		}
		
		Assert(enemy->m_Alive == XFALSE);
		Assert(enemy->m_IsAllocated == XFALSE);
		enemy->m_IsAllocated = XTRUE;

		enemy->Initialize();
		enemy->IsAlive_set(XTRUE);
		enemy->Setup(type, pos, spawnMode);
		
		m_WorldGrid.Add(enemy->Position_get(), enemy);
		m_GridEffect->ObjectAdd(enemy->Position_get());
		
		if (!IgnoreInEnemyCount(type))
		{
			m_AliveCount++;
			
#if defined(DEBUG) && 0
			LOG(LogLevel_Debug, "world: alive count: %d", m_AliveCount);
#endif
		}
		
		if (IncludeInClutterCount(type))
		{
			m_AliveClutterCount++;
		}
		
		if (type == EntityClass_Mine)
			m_AliveMineCount++;
		
		return enemy;
	}

	void World::RemoveEnemy(EntityEnemy* enemy)
	{
		Assert(enemy->m_Alive == XTRUE);
		Assert(enemy->m_IsAllocated == XTRUE);
		enemy->m_IsAllocated = XFALSE;
		
		m_WorldGrid.Remove(enemy->Position_get(), enemy);
		m_GridEffect->ObjectRemove(enemy->Position_get());
		
		enemy->IsAlive_set(XFALSE);
		
		m_enemies.Free(enemy);
		
		if (!IgnoreInEnemyCount(enemy->Class_get()))
			m_AliveCount--;
		if (IncludeInClutterCount(enemy->Class_get()))
			m_AliveClutterCount--;
		if (enemy->Class_get() == EntityClass_Mine)
			m_AliveMineCount--;
	}
	
	void World::RemoveEnemies_ByClass(EntityClass type)
	{
		for (int i = 0; i < m_enemies.PoolSize_get(); ++i)
		{
			if (!m_enemies[i].IsAlive_get())
				continue;			
			if (m_enemies[i].Class_get() != type)
				continue;
			
			RemoveEnemy(&m_enemies[i]);
		}
	}
	
	EntityEnemy* World::SpawnBadSector(const Vec2F& pos)
	{
		EntityEnemy* enemy = m_badSectors.Allocate();
		
		if (!enemy)
		{
			return 0;
		}
		
		Assert(enemy->m_Alive == XFALSE);
		Assert(enemy->m_IsAllocated == XFALSE);
		enemy->m_IsAllocated = XTRUE;

		enemy->Initialize();
		enemy->IsAlive_set(XTRUE);
		enemy->Setup(EntityClass_BadSector, pos, EnemySpawnMode_ZoomIn);
		
		return enemy;
	}
	
	void World::SpawnPowerup(PowerupType type, const Vec2F& pos, PowerupMoveType moveType)
	{
		EntityPowerup* powerup = new EntityPowerup();
		powerup->Initialize();
		powerup->IsAlive_set(XTRUE);
		powerup->Setup(type, moveType, pos, 8.0f);
		AddDynamic(powerup);
	}
	
	void World::SpawnBandit(Res* res, int level, int mods)
	{
		if (level < 0)
			level = g_GameState->m_GameRound->Classic_Level_get();
		
		Bandits::EntityBandit* bandit = new Bandits::EntityBandit();
		bandit->Initialize();
		bandit->IsAlive_set(XTRUE);
		bandit->Setup(res, level, -1.0f, mods);
		AddDynamic(bandit);
	}
	
	void World::HandleBanditDeath(Entity* bandit)
	{
		m_AliveMaxiCount--;
	}
	
	void World::SpawnZoomParticles(Vec2F position, int count)
	{
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::PARTICLE_SPARK);
		
		for (int i = 0; i < count; ++i)
		{
			Particle& p = g_GameState->m_ParticleEffect.Allocate(image->m_Info, 0, 0);
			
			Particle_Default_Setup(
				&p,
				position[0] + Calc::Random_Scaled(10.0f) * 0.0f,
				position[1] + Calc::Random_Scaled(10.0f) * 0.0f, 1.0f / 2.0f, 10.0f, 4.0f, Calc::Random(Calc::m2PI), 200.0f);
		}
	}
	
	Vec2F World::MakeSpawnPoint_OutsideWorld(float offset) const
	{
		Vec2F corners[4] =
		{
			Vec2F(0.0f - offset, 0.0f - offset),
			Vec2F(WORLD_SX + offset, 0.0f - offset),
			Vec2F(WORLD_SX + offset, WORLD_SY + offset),
			Vec2F(0.0f - offset, WORLD_SY + offset)
		};
		
		float totalLength = (WORLD_SX + offset * 2.0f) * 2.0f + (WORLD_SY + offset * 2.0f) * 2.0f;
		
		float start = Calc::Random(0.0f, totalLength);
		
		float length1 = 0.0f;

		for (int i = 0; i < 4; ++i)
		{
			int index1 = (i + 0) % 4;
			int index2 = (i + 1) % 4;
			
			Vec2F p1 = corners[index1];
			Vec2F p2 = corners[index2];
			
			Vec2F delta = p2 - p1;
			
			float length = delta.Length_get();
			
			float length2 = length1 + length;
			
			if (start <= length2)
			{
				float t = (start - length1) / (length2 - length1);
				
				return p1.LerpTo(p2, t);
			}
			
			length1 = length2;
		}
		
		return Vec2F(-offset, -offset);
	}
	
	int World::AliveEnemiesCount_get() const 
	{
		return m_AliveCount;
	}
	
	int World::AliveMiniCount_get() const 
	{
		return m_AliveMiniCount; 
	}
	
	int World::AliveMaxiCount_get() const 
	{
		return m_AliveMaxiCount; 
	}
	
	int World::AliveMineCount_get() const
	{
		return m_AliveMineCount;
	}
	
	int World::AliveClutterCount_get() const
	{
		return m_AliveClutterCount;
	}
			
	bool World::IgnoreInEnemyCount(EntityClass type) const
	{
//		if (type == EntityClass_BadSector)
//			return true;
		
		if (g_GameState->m_GameRound->GameMode_get() == Game::GameMode_ClassicLearn)
			return false;
		
		switch (type)
		{
			case EntityClass_Mine:
			case EntityClass_BorderPatrol:
			case EntityClass_EvilSquare:
			case EntityClass_EvilSquareBiggy:
			case EntityClass_EvilSquareBiggySmall:
//			case EntityClass_EvilTriangleBiggy: // note: triangle biggy follows player - so wait 'till it's been destroyed
			case EntityClass_EvilTriangleBiggySmall:
				return true;
			default:
				return false;
		}
	}
	
	bool World::IncludeInClutterCount(EntityClass type) const
	{
		return type == EntityClass_EvilSquare || type == EntityClass_EvilSquareBiggy;
	}

	void World::HandleKill(Entity* entity)
	{
		m_Player->HandleKill(entity->Class_get());
	}
	
	//
	
	void World::HandleWaveBegin(void* obj, void* arg)
	{
		World* self = (World*)obj;
		
		self->m_IntermezzoMgr.Start(IntermezzoType_WaveBanner);
	}
	
	void World::HandleBossBegin(void* obj, void* arg)
	{
	}
	
	void World::HandleLevelCleared(void* obj, void* arg)
	{
		World* self = (World*)obj;
		
		self->m_IntermezzoMgr.Start(IntermezzoType_LevelBegin);
	}
	
	void World::HandleGameEnd()
	{
		// switch to intro mode & activate main view
		
		g_GameState->m_GameRound->Setup(GameMode_IntroScreen, g_GameState->m_GameRound->Modifier_Difficulty_get());
		
		g_GameState->ActiveView_set(::View_Main);
	}
	
	void World::HandleGameOver(void* obj, void* arg)
	{
		// clear save game, switch to intro mode & activate game over view
		
		g_GameState->m_GameSave->Clear();
		
		g_GameState->m_GameRound->Setup(GameMode_IntroScreen, g_GameState->m_GameRound->Modifier_Difficulty_get());

		g_GameState->ActiveView_set(::View_GameOver);
	}
	
	// --------------------
	// Logic
	// --------------------
	
	void World::IsPaused_set(bool paused)
	{
		if (paused == m_IsPaused)
			return;
		
		m_IsPaused = paused;
		
		g_GameState->m_SoundEffectMgr->MuteLoops(paused);
	}
	
	bool World::IsPaused_get() const
	{
		return m_IsPaused;
	}
	
	// --------------------
	// Controller pause
	// --------------------
	
	void World::IsControllerPaused_set(bool paused)
	{
		if (paused == m_IsControllerPaused)
			return;
		
		m_IsControllerPaused = paused;
		
		if (paused)
		{
			m_ControllerPausedScalar = 1.0f;
			m_ControllerPausedAnim.Start(AnimTimerMode_TimeBased, true, 0.3f, AnimTimerRepeat_Mirror);
		}
		else
		{
			m_ControllerPausedAnim.Stop();
		}
	}
	
	//
	
	void World::CameraOverride_set(bool value)
	{
		m_CameraOverride = value;
		
		if (m_CameraOverride)
		{
			// todo: update touch zoom controller with current camera focus/zoom
		}
	}
	
	// Entities
	
	void World::AddDynamic(Entity* entity)
	{
		if (entity->Flag_IsSet(EntityFlag_IsMiniBoss))
		{
			m_AliveMiniCount++;
		}
		
		if (entity->Flag_IsSet(EntityFlag_IsMaxiBoss))
		{
			m_AliveMaxiCount++;
		}
		
		m_dynamics.Add(entity);
	}
	
	void World::HandleDynamicRemove(void* obj, void* arg)
	{
		World* self = (World*)obj;
		Entity* entity = (Entity*)arg;
		
		if (entity->Flag_IsSet(EntityFlag_IsMiniBoss))
			self->m_AliveMiniCount--;
		if (entity->Flag_IsSet(EntityFlag_IsMaxiBoss))
		{
			if (!entity->Flag_IsSet(Game::EntityFlag_Transient))
				self->m_AliveMaxiCount--;
		}
	}
	
	void World::ForEachDynamic(CallBack cb)
	{
		m_dynamics.ForEach(cb);
	}
	
	class ForEach_InArea_Query
	{
	public:
		RectF rect;
		CallBack cb;
	};
	
	static void HandleForEach_InArea2(void* obj, void* arg)
	{
		ForEach_InArea_Query* query = (ForEach_InArea_Query*)arg;
		Entity* entity = (Entity*)obj;
		
		query->cb.Invoke(entity);
	}
	
	static void HandleForEach_InArea(void* obj, void* arg)
	{
		ForEach_InArea_Query* query = (ForEach_InArea_Query*)obj;
		Entity* entity = (Entity*)arg;
		
		const Vec2F pos = entity->Position_get();
		
		if (query->rect.IsInside(pos))
			query->cb.Invoke(entity);
	}
	
	void World::ForEach_InArea(Vec2F min, Vec2F max, CallBack cb)
	{
		ForEach_InArea_Query query;
		query.rect = RectF(min, max - min);
		query.cb = cb;
		
		m_WorldGrid.ForEach_InArea(min, max, HandleForEach_InArea2, &query);
		ForEachDynamic(CallBack(&query, HandleForEach_InArea));
	}

	// --------------------
	// Bad sector
	// --------------------
	void World::Update_HeartBeat()
	{
		if (m_AliveMineCount == 0)
			return;
		
		if (m_HeartBeatTrigger.Read())
		{
			g_GameState->m_SoundEffects->Play(Resources::SOUND_MINE_HEARTBEAT, SfxFlag_MustFinish);
			
			m_HeartBeatTrigger.Start(HeartBeat_GetInterval());
		}
	}
	
	float World::HeartBeat_GetInterval()
	{
		float minProgress = -1.0f;
		
		for (int i = 0; i < m_enemies.PoolSize_get(); ++i)
		{
			if (!m_enemies[i].IsAlive_get())
				continue;
			if (m_enemies[i].Class_get() != EntityClass_Mine)
				continue;
			
			if (m_enemies[i].MineProgress_get() < minProgress || minProgress < 0.0f)
				minProgress = m_enemies[i].MineProgress_get();
		}
		
		const float vBase = ((SoundEffectInfo*)g_GameState->GetSound(Resources::SOUND_MINE_HEARTBEAT)->info)->Duration_get() * 0.7f;
		
		const float v1 = 6.0f;
		const float v2 = 0.0f;
		
		if (minProgress <= 0.0f)
			return vBase;
		
		return vBase + v1 + (v2 - v1) * (1.0f - minProgress);
	}
	
	//
	
#if defined(DEBUG) && 0
	static void DBG_SpawnPowerups()
	{
		int moveType = PowerupMoveType_WaveUp;
		{
			for (int type = PowerupType_Undefined + 1; type < PowerupType__Count; ++type)
			{
				g_World->SpawnPowerup((PowerupType)type, WORLD_MID + Vec2F(-80.0f + type * 80.0f, 40.0f + (moveType + 1) * 80.0f), (PowerupMoveType)moveType);
				g_World->SpawnPowerup(PowerupType_Credits, WORLD_MID + Vec2F(-80.0f + type * 80.0f, 40.0f + 80.0f), (PowerupMoveType)moveType);				
			}
		}
	}
	
	static void DBG_SpawnMiniBosses()
	{
		g_World->m_Bosses.Spawn(BossType_Magnet, 1);
	}
	
	static void DBG_BadSectorsGalore()
	{
		g_World->m_SectorGrid.DBG_DestroyAll();
	}
#endif
};
