#include "BanditEntity.h"
#include "Calc.h"
#include "EntityPlayer.h"
#include "GameHelp.h"
#include "GameRound.h"
#include "GameSave.h"
#include "GameSettings.h"
#include "GameState.h"
#include "Grid_Effect.h"
#include "Mat3x2.h"
#include "RandomPicker.h"
#include "SoundEffectMgr.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "World.h"

#define ROUND_WAIT 1.0f // time at start of level (player spawn / idle time)
#define WAVE_WAIT 3.0f // time between waves
#define WAVE_WAIT_TUTORIAL 0.5f // time between waves (tutorial mode)
//#define MINI_WAIT 1.0f
#define MAXI_WAIT 0.2f // time between maxi phase and spawn (ooh the suspense!)
#define LEVEL_WAIT 4.0f // time between level cleared and next round (allows for bandit explosions)

#define SPAWN_INTERVAL 0.15f

#define INVADERS_SPACE_X 35.0f
#define INVADERS_SPACE_Y 35.0f

namespace Game
{
	GameRound::GameRound()
	{
		Initialize();
	}

	GameRound::~GameRound()
	{
	}
	
	void GameRound::Initialize()
	{
		m_Log = LogCtx("RoundController");
		
		m_GameMode = GameMode_IntroScreen;
		
		m_WaveTimer.Initialize(g_GameState->m_TimeTracker_World);
		
		m_RoundCoolDownTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_WaveCoolDownTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_MaxiCoolDownTimer.Initialize(g_GameState->m_TimeTracker_World);
		m_LevelCoolDownTimer.Initialize(g_GameState->m_TimeTracker_World);
		
		m_TriangleHueTimer.Initialize(&m_TriangleHueTime, false);
		
		m_TriangleHueTimer.Start(AnimTimerMode_TimeBased, false, 40.0f, AnimTimerRepeat_Mirror);

		// v1.3
		m_CustomWaveCount = 0;

		m_GameType = GameType_Classic;
		
		m_LoveNotWar = false;
	}
	
	void GameRound::Clear()
	{
		m_WaveTimer.Stop();
		m_RoundCoolDownTimer.Stop();
		m_WaveCoolDownTimer.Stop();
		m_MaxiCoolDownTimer.Stop();
		m_LevelCoolDownTimer.Stop();
		
		m_EnemyWaveMgr.Clear();
		
		m_WaveInfo.Initialize();
		
		m_RoundState = RoundState_Idle;
		m_WaveSpawnState = SpawnState_Idle;
		m_MaxiSpawnState = SpawnState_Idle;

		m_CustomWaveCount = 0;
	}
	
	void GameRound::Setup(GameMode mode, Difficulty difficulty)
	{
		Assert(difficulty != Difficulty_Unknown);
		
		m_GameMode = mode;
		
		m_WaveInfo.difficulty = difficulty;

#ifndef DEPLOYMENT
#pragma comment("love not war")
		m_LoveNotWar = false; // fixme!
#else
		m_LoveNotWar = false;
#endif
		
		if (GameModeIsClassic())
		{
			Classic_RoundState_set(RoundState_Idle);
		}
		if (GameModeIsInvaders())
		{
			m_InvadersInfo.Initialize();
		}
	}
	
	//
	
	void GameRound::Update(float dt)
	{
		const float hue = g_World->m_GridEffect->m_BaseHueController.Angle_get() / Calc::m2PI;
		m_WaveInfo.maxiColor = Calc::Color_FromHue(hue);
		
		switch (m_GameMode)
		{
			case GameMode_IntroScreen:
				IntroScreen_Update(dt);
				break;
			case GameMode_ClassicLearn:
			case GameMode_ClassicPlay:
				Classic_Update(dt);
				break;
			case GameMode_InvadersPlay:
				Invaders_Update(dt);
				break;
		}
	}
	
	//
	
	void GameRound::IntroScreen_Update(float dt)
	{
		AssertIsIntroScreen();
		
		// update spawnage of enemy waves
		
		m_EnemyWaveMgr.Update(dt);
	}
	
	EntityClass GameRound::IntroScreen_GetEnemyType() const
	{
		RandomPicker<EntityClass, 20> picker;
		
		picker.Add(EntityClass_EvilTriangle, 1.0f);
		picker.Add(EntityClass_Kamikaze, 1.0f);
		picker.Add(EntityClass_EvilTriangleBiggy, 1.0f);
		picker.Add(EntityClass_EvilSquare, 1.0f);
		picker.Add(EntityClass_EvilSquareBiggy, 1.0f);
		picker.Add(EntityClass_Shield, 1.0f);
		
		return picker.Get();
	}
	
	//
	
	void GameRound::Classic_Update(float dt)
	{
#ifndef DEPLOYMENT
		if (Modifier_MakeLoveNotWar_get())
		{
#pragma message("todo: use this bit of code to init heart mode")
			for (int type = EntityClass__SmallFry_Begin + 1; type < EntityClass__SmallFry_End; ++type)
			{
//				ReplaceEnemiesByType((EntityClass)type, EntityClass_Smiley);
			}
		}
#endif
		
		// update spawnage of enemy waves
		
		m_EnemyWaveMgr.Update(dt);
	
		//
		
		if (m_RoundState == Game::RoundState_PlayMaxiBoss)
			g_World->m_GridEffect->EffectType_set(Game::GridEffectType_Dark);
		else
			g_World->m_GridEffect->EffectType_set(Game::GridEffectType_Colors);
		
		//
		
		switch (m_RoundState)
		{
			case RoundState_Idle:
			{
				if (m_RoundCoolDownTimer.ReadTick())
				{
					Classic_RoundState_set(RoundState_PlayWaves);
				}
				break;
			}
			
			case RoundState_PlayWaves:
			{
				// check wave state, spawn appropriately

				if(!g_GameState->m_GameSettings->m_CustomSettings.Wave_Toggle)
				{
					Classic_RoundState_set(RoundState_PlayMaxiBoss);
					break;
				}
				
				switch (m_WaveInfo.waveMode)
				{
					case WaveMode_AliveCount:
					{
						switch (m_WaveSpawnState)
						{
							case SpawnState_Idle:
							{
								break;
							}
							
							case SpawnState_CoolDown:
							{
								if (m_WaveCoolDownTimer.ReadTick())
								{
									m_WaveCoolDownTimer.Stop();
									
									Classic_WaveSpawnState_set(SpawnState_Spawn);
								}
								break;
							}
								
							case SpawnState_Spawn:
							{
								Classic_WaveSpawnState_set(SpawnState_Wait);
								break;
							}
							
							case SpawnState_Wait:
							{
								if (Classic_HasReachedWaveTreshold())
								{
									if (!Classic_HasReachedWaveFinal())
									{
										Classic_WaveSpawnState_set(SpawnState_CoolDown);
									}
									else if (Classic_HasWonWaveAll())
									{
										Classic_WaveSpawnState_set(SpawnState_Done);
									}
								}
								
								break;
							}
							
							case SpawnState_Done:
							{
								break;
							}
						}
						
						break;
					}
						
					case WaveMode_Time:
					{
						switch (m_WaveSpawnState)
						{
							case SpawnState_Idle:
								break;
							
							case SpawnState_CoolDown:
							{
								if (m_WaveCoolDownTimer.ReadTick())
								{
									m_WaveCoolDownTimer.Stop();
									
									Classic_WaveSpawnState_set(SpawnState_Spawn);
								}
								break;
							}
								
							case SpawnState_Spawn:
							{
								Classic_WaveSpawnState_set(SpawnState_Wait);
								break;
							}
							
							case SpawnState_Wait:
							{
								if (!Classic_HasReachedWaveFinal())
								{
									if (m_WaveTimer.ReadTick())
									{
										Classic_WaveSpawnState_set(SpawnState_CoolDown);
									}
								}
								
								if (Classic_HasWonWaveAll())
								{
									Classic_WaveSpawnState_set(SpawnState_Done);
								}
								
								break;
							}
							
							case SpawnState_Done:
							{
								break;
							}
						}

						break;
					}
				}
				break;
			}
			
			case RoundState_PlayMaxiBoss:
			{
				switch (m_MaxiSpawnState)
				{
					case SpawnState_Idle:
						break;
						
					case SpawnState_CoolDown:
					{
						if (m_MaxiCoolDownTimer.ReadTick())
						{
							m_MaxiCoolDownTimer.Stop();
							
							Classic_MaxiSpawnState_set(SpawnState_Spawn);
						}
						
						break;
					}
						
					case SpawnState_Spawn:
					{
						break;
					}
						
					case SpawnState_Wait:
					{
						if (Classic_HasWonMaxiBoss())
						{
							Classic_MaxiSpawnState_set(SpawnState_Done);
						}
						
						break;
					}
						
					case SpawnState_Done:
					{
						break;
					}
				}

				break;
			}
				
			case RoundState_LevelCleared:
			{
				if (m_LevelCoolDownTimer.ReadTick())
				{
					m_LevelCoolDownTimer.Stop();
					
					Classic_HandleLevelWin();
				}
				break;
			}
		}
	}
	
	// ----------------------------------------
	// Invaders Game Mode
	// ----------------------------------------
	
	void GameRound::Invaders_Update(float dt)
	{
		InvadersInfo& info = m_InvadersInfo;
		
		switch (info.state)
		{
			case InvadersState_Idle:
			{
				Invaders_NextLevel();
			}
			break;
			case InvadersState_Play:
			{
				Invaders_UpdateEnemyPositions(dt);
				
				bool win = Invaders_CheckLevelWin();
				
				if (win)
				{
					Invaders_HandleLevelWin();
				}
				else
				{
					if (Invaders_GetSpawnUfo())
					{
						Invaders_NextUfo();
					}
				}
			}	
			break;
			case InvadersState_LevelWon:
			{
				info.state = InvadersState_Idle;
			}
			break;
		}
	}
	
	void GameRound::Invaders_UpdateEnemyPositions(float dt)
	{
		InvadersInfo& info = m_InvadersInfo;
		
		// update offsets
		
		info.offsetY = 0.0f;
		
		info.moveTimer -= dt;
		
		if (info.moveTimer <= 0.0f)
		{
			info.moveTimer = 1.0f;
			info.move++;
		}
		
		const int numMoves = 8;
		int step = info.move % numMoves;
		int offset[numMoves] = { 0, -1, -2, -1, 0, +1, +2, +1 };
		int dx = offset[step];
		int dy = info.move / numMoves / 5;
		
		info.offsetY = dy * INVADERS_SPACE_Y;
		for (int y = 0; y < INVADERS_ENEMIES_SY; ++y)
			info.offsetX[y] = dx * INVADERS_SPACE_X;
		
		//for (int y = 0; y < INVADERS_ENEMIES_SY; ++y)
		//	info.offsetX[y] = sinf(g_TimerRT.Time_get()) * 50.0f;								
	}
	
    void GameRound::Invaders_HandleEnemyDeath(Entity* enemy)
	{
		InvadersInfo& info = m_InvadersInfo;
		
		Assert(info.enemyCount > 0);
		
		for (int y = 0; y < INVADERS_ENEMIES_SY; ++y)
		{
			for (int x = 0; x < INVADERS_ENEMIES_SX; ++x)
			{
				if (info.enemies[y][x] == enemy)
					info.enemies[y][x] = 0;
			}
		}
		
		info.enemyCount--;
	}
    
	Vec2F GameRound::Invaders_GetEnemyPos(int x, int y)
	{
		InvadersInfo& info = m_InvadersInfo;
		
		const float spaceX = INVADERS_SPACE_X;
		const float spaceY = INVADERS_SPACE_Y;
		
		const float x1 = VIEW_SX / 2.0f - spaceX * (INVADERS_ENEMIES_SX - 1) / 2.0f;
		const float x2 = VIEW_SX / 2.0f + spaceX * (INVADERS_ENEMIES_SX - 1) / 2.0f;
		const float y1 = 50.0f;
		const float y2 = y1 + INVADERS_ENEMIES_SY * spaceY;
		
		const float interpX = x / (INVADERS_ENEMIES_SX - 1.0f);
		const float interpY = y / (INVADERS_ENEMIES_SY - 1.0f);
		
		const float posX = x1 * (1.0f - interpX) + x2 * interpX + info.offsetX[y];
		const float posY = y1 * (1.0f - interpY) + y2 * interpY + info.offsetY;
		
		return Vec2F(posX, posY);
	}
	//
	
	void GameRound::Invaders_DefenseRestore()
	{
		// todo: restore defense
	}
	
	void GameRound::Invaders_DefenseHarden()
	{
		// todo: upgrade defensive strength
	}
	
	//
	
	void GameRound::Invaders_HandleLevelWin()
	{
		InvadersInfo& info = m_InvadersInfo;
		
		info.state = InvadersState_LevelWon;
	}
	
	//
	
	bool GameRound::Invaders_CheckLevelWin()
	{
		InvadersInfo& info = m_InvadersInfo;
		
		return info.enemyCount == 0;
	}
	
	//
	
	void GameRound::Invaders_NextLevel()
	{
		InvadersInfo& info = m_InvadersInfo;
		
		// reset stuff
		
		info.InitializeLevel();
		
		// spawn enemies
		
		for (int y = 0; y < INVADERS_ENEMIES_SY; ++y)
		{
			EntityClass type = Invaders_GetEnemyType(y);
			
			for (int x = 0; x < INVADERS_ENEMIES_SX; ++x)
			{
				Vec2F pos = Invaders_GetEnemyPos(x, y);

				EntityEnemy* entity = (EntityEnemy*)g_World->SpawnEnemy(type, pos, EnemySpawnMode_DropDown);
				
				entity->Invader_Setup(x, y);
				
				info.enemies[y][x] = entity;
				
				info.enemyCount++;
			}
		}
		
		info.moveTimer = 1.0f;
		
		info.state = InvadersState_Play;
	}
	
	void GameRound::Invaders_NextUfo()
	{
		BossType type = (rand() % 1) == 0 ? BossType_Snake : BossType_Magnet;
		
		BossBase* boss = g_World->m_Bosses.Spawn(type, m_InvadersInfo.level);
		
		float posX = static_cast<float>(Calc::Random(0, VIEW_SX));
		float posY = -100.0f;
		
		boss->Position_set(Vec2F(posX, posY));
	}
	
	//
	
	bool GameRound::Invaders_GetSpawnUfo()
	{
		int fps = 60;
		int interval = 20;
		int frames = interval * fps;
		return (rand() % frames) == 0;
	}
	
	EntityClass GameRound::Invaders_GetEnemyType(int row)
	{
		return EntityClass_Invader;
	}
	
	bool GameRound::Invaders_GetEnemyFire()
	{
		int fps = 60;
		int numEnemies = INVADERS_ENEMIES_SX * INVADERS_ENEMIES_SY;
		float numPerSecond = 2;
		int n = static_cast<int>(fps * numEnemies / numPerSecond);
		if (n == 1)
			n = 1;
		return (rand() % n) == 0;
	}
	
	float GameRound::Invaders_GetEnemyFireDamage()
	{
		return 10.0f * 0.9f;
	}
	
	float GameRound::Invaders_GetEnemyFireSpeed()
	{
		return 100.0f;
	}
	
	//
	
	void GameRound::RenderAdditive()
	{
		m_EnemyWaveMgr.Render();
	}
	
	// ----------------------------------------
	// Classic Game Mode
	// ----------------------------------------
	
	void GameRound::Classic_Level_set(int level)
	{
		AssertIsclassic();
		
		m_WaveInfo.level = level;

		Classic_UpdateBGM();
	}
	
	int GameRound::Classic_Level_get() const
	{
		AssertIsclassic();
		
		return m_WaveInfo.level;
	}
	
	int GameRound::Classic_Wave_get() const
	{
		AssertIsclassic();
		
		return m_WaveInfo.wave;
	}
	
	int GameRound::Classic_WaveCount_get() const
	{
		AssertIsclassic();
		
		return m_WaveInfo.waveCount;
	}
	
	void GameRound::Classic_WaveAdd(EnemyWave& wave)
	{
		AssertIsclassic();
		
		m_EnemyWaveMgr.Add(wave);
	}
	
	//
	
	void GameRound::Classic_HandleLevelBegin()
	{
		// advance level
		
		Classic_NextLevel();
		
		// spawn mini boss
		
		Classic_NextMiniBoss();
		
		// spawn bad sector
		
		if (g_GameState->m_GameSettings->m_CustomSettings.Mines_Toggle == true)
			Classic_NextBadSector();
		
		if (m_GameMode == GameMode_ClassicLearn)
		{
			// wait for action
			
			Classic_WaveSpawnState_set(SpawnState_Wait);
		}
		else
		{
			// begin spawning waves
			
			Classic_WaveSpawnState_set(SpawnState_CoolDown);
		}
	}
	
	void GameRound::Classic_HandleLevelWin()
	{
		m_WaveInfo.levelWon = true;
		
		// start all over
		
		Classic_RoundState_set(RoundState_Idle);
		
		// show bandit intro view

		if(m_WaveInfo.difficulty  == Difficulty_Custom)
		{
			if(g_GameState->m_GameSettings->m_CustomSettings.Boss_Toggle)
				g_GameState->ActiveView_set(::View_BanditIntro);
			else
				g_GameState->ActiveView_set(::View_InGame);
		}
		else
			g_GameState->ActiveView_set(::View_BanditIntro);
	}
	
	void GameRound::Classic_HandleWaveBegin()
	{
		// advance wave
		
		Classic_NextWave();
	}
	
	void GameRound::Classic_HandleWaveWinAll()
	{
		// play the maxi boss
			
		if (g_GameState->m_GameSettings->m_CustomSettings.Boss_Toggle)
			Classic_RoundState_set(RoundState_PlayMaxiBoss);
		else
		{
			Classic_HandleLevelBegin();
		}
	}
	
	void GameRound::Classic_HandleMaxiBossBegin()
	{
		// spawn maxi boss
		
		Classic_NextMaxiBoss();
		
		//
		
		if (Classic_OnMaxiBossBegin.IsSet())
			Classic_OnMaxiBossBegin.Invoke(this);
		
		//
		
		Classic_MaxiSpawnState_set(SpawnState_Wait);
	}
	
	void GameRound::Classic_HandleMaxiBossWin()
	{
		Classic_RoundState_set(RoundState_LevelCleared);
	}
	
	void GameRound::Classic_HandleHelpComplete()
	{
		g_World->RemoveEnemies_ByClass(EntityClass_WaveStaller);
	}
	
	//
	
	void GameRound::Classic_RoundState_set(RoundState state)
	{
		// validate state transition is valid
		
		switch (state)
		{
			case RoundState_Idle:
				break;
			case RoundState_PlayWaves:
				Assert(m_RoundState == RoundState_Idle || m_RoundState == RoundState_LevelCleared);
				break;
			case RoundState_PlayMaxiBoss:
				Assert(m_RoundState == RoundState_PlayWaves);
				break;
			case RoundState_LevelCleared:
				Assert(m_RoundState == RoundState_PlayMaxiBoss || m_RoundState == RoundState_PlayWaves);
				break;
		}
		
		m_RoundState = state;
		
		// generate events due to state transition
		
		switch (state)
		{
			case RoundState_Idle:
			{
				m_Log.WriteLine(LogLevel_Debug, "State change: Idle");
				m_RoundCoolDownTimer.SetInterval(ROUND_WAIT);
				m_RoundCoolDownTimer.Start();
				Classic_HandleLevelBegin();
				break;
			}
			case RoundState_PlayWaves:
			{
				m_Log.WriteLine(LogLevel_Debug, "State change: Play Waves");
//				HandleLevelBegin();
				break;
			}
			case RoundState_PlayMaxiBoss:
			{
				m_Log.WriteLine(LogLevel_Debug, "State change: Play Maxi Boss");
				
				// select base hue for grid effect
				
				g_World->m_GridEffect->BaseHue_set(m_WaveInfo.maxiHue - 0.05f); // apply a small shift in hue to create a little contrast between the maxi laser and the background

				// start maxi boss spawning
				
				Classic_MaxiSpawnState_set(SpawnState_CoolDown);
				
				m_MaxiCoolDownTimer.SetInterval(MAXI_WAIT);
				m_MaxiCoolDownTimer.Start();
				break;
			}
			case RoundState_LevelCleared:
			{
				m_Log.WriteLine(LogLevel_Debug, "State change: Level Cleared");
				if (Classic_OnLevelCleared.IsSet())
					Classic_OnLevelCleared.Invoke(this);
				m_LevelCoolDownTimer.SetInterval(LEVEL_WAIT);
				m_LevelCoolDownTimer.Start();
				break;
			}
		}
	}
	
	void GameRound::Classic_WaveSpawnState_set(SpawnState state)
	{
		m_WaveSpawnState = state;
		
		switch (m_WaveSpawnState)
		{
			case SpawnState_Idle:
			{
				break;
			}
				
			case SpawnState_CoolDown:
			{
				if (m_GameMode == GameMode_ClassicLearn)
					m_WaveCoolDownTimer.SetInterval(WAVE_WAIT_TUTORIAL);
				else
					m_WaveCoolDownTimer.SetInterval(WAVE_WAIT);
				m_WaveCoolDownTimer.Start();
				break;
			}
				
			case SpawnState_Spawn:
			{
				Classic_HandleWaveBegin();
				
				if (Classic_OnWaveBegin.IsSet())
					Classic_OnWaveBegin.Invoke(this);
				break;
			}
				
			case SpawnState_Wait:
			{
				break;
			}
				
			case SpawnState_Done:
			{
				Classic_HandleWaveWinAll();
				break;
			}
		}
	}
	
	void GameRound::Classic_MaxiSpawnState_set(SpawnState state)
	{
		m_MaxiSpawnState = state;
		
		switch (m_MaxiSpawnState)
		{
			case SpawnState_Idle:
			{
				break;
			}
			case SpawnState_CoolDown:
			{
				m_MaxiCoolDownTimer.SetInterval(MAXI_WAIT);
				m_MaxiCoolDownTimer.Start();
				break;
			}
			case SpawnState_Spawn:
			{
				Classic_HandleMaxiBossBegin();
				break;
			}
			case SpawnState_Wait:
			{
				break;
			}
			case SpawnState_Done:
			{
				Classic_HandleMaxiBossWin();
				break;
			}
		}
	}

	//
	
	bool GameRound::Classic_HasReachedWaveTreshold() const
	{
		AssertIsclassic();
		
		// all enemies must have been spawned
		
		if (!m_EnemyWaveMgr.IsEmpty_get())
			return false;
		
		if (m_GameMode == GameMode_ClassicLearn)
		{
			return m_WaveInfo.learn_waveWon;
		}
		else
		{
			// alive treshold must have been met
			
			if (m_WaveInfo.levelType == LevelType_Regular || m_WaveInfo.levelType == LevelType_EnemyAttack)
			{
				if (g_World->AliveEnemiesCount_get() > m_WaveInfo.waveAliveTreshold)
					return false;
			}
			if (m_WaveInfo.levelType == LevelType_MiniAttack)
			{
				if (g_World->AliveMiniCount_get() > 0)
					return false;
			}
			
			return true;
		}
	}
	
	bool GameRound::Classic_HasReachedWaveFinal() const
	{
		AssertIsclassic();
		
		if (m_GameMode == GameMode_ClassicLearn)
		{
			return m_WaveInfo.learn_levelWon;
		}
		else
		{
			return m_WaveInfo.wave + 1 >= m_WaveInfo.waveCount;
		}
	}
	 
	bool GameRound::Classic_HasWonWaveAll() const
	{
		AssertIsclassic();
		
		// all enemies must have been spawned
		
		if (!m_EnemyWaveMgr.IsEmpty_get())
			return false;
		
		// all enemies must have been killed
		
		if (g_World->AliveEnemiesCount_get() > 0)
			return false;
		
		/*
		// sufficient mini bosses must be killed in mini boss attack level
		
		if (m_WaveInfo.levelType == LevelType_MiniAttack)
			if (g_World->AliveMiniCount_get() > 1)
				return false;
		*/
		
		if (m_GameMode == GameMode_ClassicLearn)
		{
			// level won flag must be set

			if (m_WaveInfo.learn_levelWon == false)
				return false;
		}
		else
		{
			// final wave must have been finished
			
			if (!Classic_HasReachedWaveFinal())
				return false;
		}

		return true;
	}
	
	bool GameRound::Classic_HasWonMaxiBoss() const
	{
		AssertIsclassic();
		
		return g_World->AliveMaxiCount_get() == 0;
	}
	
	bool GameRound::Classic_HasWonLevel() const
	{
		AssertIsclassic();
		
		return m_RoundState == RoundState_PlayMaxiBoss && Classic_HasWonMaxiBoss();
	}
	
	//
	
	void GameRound::Classic_NextWave()
	{
		AssertIsclassic();
		
		if (m_WaveInfo.wave == 0 && (m_WaveInfo.levelType == LevelType_EnemyAttack || m_WaveInfo.levelType == LevelType_MiniAttack))
		{
			m_WaveInfo.levelType = LevelType_Regular;
		}
		
		m_WaveInfo.wave++;

		m_CustomWaveCount++;
		
		// select theme
		
		RoundTheme theme = Classic_GetRoundTheme();
		
		RoundTheme_set(theme);
		
		if (m_WaveInfo.levelType == LevelType_Regular)
		{
			// spawn enemies
			
			EntityClass type = Classic_GetEnemyType();
			
			int count = Classic_GetEnemyCount();
			
			SpawnFormation formation = Classic_GetEnemySpawnFormation(count);
			
			Spawn(type, formation, g_Target, count);
			
			if (m_GameMode == GameMode_ClassicPlay)
			{
				// spawn clutter at random places from time to time
			
				if (g_GameState->m_GameSettings->m_CustomSettings.Clutter_Toggle == true)
				{
					if (Classic_GetSpawnClutter())
					{
						Vec2F pos(Calc::Random((float)WORLD_SX), Calc::Random((float)WORLD_SY));
						EntityClass clutterType = Classic_GetClutterType();
						Spawn(clutterType, SpawnFormation_Cluster, pos, Classic_GetClutterCount());
					}
				}
			
				// spawn mines from time to time
				if (g_GameState->m_GameSettings->m_CustomSettings.Mines_Toggle == true)
				{
					if (Classic_GetSpawnMines())
					{
						EnemyWave wave;
						wave.MakeCluster(EntityClass_Mine, g_World->m_Player->Position_get(), 100.0f, Classic_GetSpawnMineCount(), SPAWN_INTERVAL);
						m_EnemyWaveMgr.Add(wave);
					}
					
					// spawn mines at random places from time to time
					
					if (Classic_GetSpawnRandomMines())
					{
						const float borderSize = 60.0f;
						Vec2F size(WORLD_SX - borderSize * 2.0f, WORLD_SY - borderSize * 2.0f);
						Vec2F pos = Vec2F(borderSize, borderSize) + Vec2F(Calc::Random(size[0]), Calc::Random(size[1]));
						
						EnemyWave wave;
						wave.MakeCluster(EntityClass_Mine, pos, 40.0f, Classic_GetSpawnRandomMineCount(), SPAWN_INTERVAL);
						m_EnemyWaveMgr.Add(wave);
					}
				}
			}
			
			if(g_GameState->m_GameSettings->m_CustomSettings.Boss_Toggle == false)
			{
				if (m_CustomWaveCount % 8 == 0)
				{
					g_World->SpawnBandit(g_GameState->m_ResMgr.Get(Resources::BANDIT_GUNSHIP), m_WaveInfo.level, Bandits::BanditMod_AvoidBandits);
					//m_WaveInfo.level++;
				}
			}
			else if (m_WaveInfo.wave == 0)
			{
				// spawn support bandit
				
				int start1 = m_WaveInfo.difficulty == Difficulty_Easy ? 7 : 7;
				int start2 = m_WaveInfo.difficulty == Difficulty_Easy ? 15 : 15;
				
				if (m_WaveInfo.level >= start1)
					g_World->SpawnBandit(g_GameState->m_ResMgr.Get(Resources::BANDIT_GUNSHIP), m_WaveInfo.level - start1, Bandits::BanditMod_AvoidBandits);
				if (m_WaveInfo.level >= start2)
					g_World->SpawnBandit(g_GameState->m_ResMgr.Get(Resources::BANDIT_GUNSHIP), m_WaveInfo.level - start2, Bandits::BanditMod_AvoidBandits);
			}
		}
		
		if (m_WaveInfo.levelType == LevelType_EnemyAttack)
		{
			EntityClass type = Classic_GetEnemyType();
			
			// note: only spawn enemies which attack player head-on
			
			// todo: use enemy count getter?
			
			for (int i = 0; i < 40; ++i)
			{
				g_World->SpawnEnemy(type, g_World->MakeSpawnPoint_OutsideWorld(200.0f), EnemySpawnMode_Instant);
			}
		}

		if (m_GameMode == GameMode_ClassicLearn)
		{
			m_WaveInfo.learn_waveWon = false;
		}
	}
	
	void GameRound::Classic_NextMiniBoss()
	{
		AssertIsclassic();
		
		// spawn boss

		// custom HACK
		if (!g_GameState->m_GameSettings->m_CustomSettings.PowerUp_Toggle)
			return;

		for (int i = 0; i < m_WaveInfo.bossCount; ++i)
		{
			g_World->m_Bosses.Spawn(Classic_GetMiniBossType(), m_WaveInfo.level);
		}
	}
	
	void GameRound::Classic_NextMaxiBoss()
	{
		AssertIsclassic();
		
		// play scary sound
		
		g_GameState->m_SoundEffects->Play(Resources::SOUND_BOSS_ALARM_BOSS, SfxFlag_MustFinish);
		
		// spawn bandit
		
		g_World->SpawnBandit(g_GameState->m_ResMgr.Get(Classic_GetMaxiBossType()), m_WaveInfo.level, 0);
		
		// spawn support bandits
		
		int start1 = m_WaveInfo.difficulty == Difficulty_Hard ? 10 : 20;
		int start2 = m_WaveInfo.difficulty == Difficulty_Hard ? 20 : 40;
		
		if (m_WaveInfo.level >= start1)
			g_World->SpawnBandit(g_GameState->m_ResMgr.Get(Resources::BANDIT_GUNSHIP), m_WaveInfo.level - start1, Bandits::BanditMod_AvoidBandits);
		if (m_WaveInfo.level >= start2)
			g_World->SpawnBandit(g_GameState->m_ResMgr.Get(Resources::BANDIT_GUNSHIP), m_WaveInfo.level - start2, Bandits::BanditMod_AvoidBandits);
		
		// show a little help message on the first level
		
		if (GameMode_get() == GameMode_ClassicLearn)
		{
			g_GameState->m_GameHelp->WriteBegin(6.0f, "ULTRA BOSS", HelpColors::MaxiCaption);
			g_GameState->m_GameHelp->WriteLine(SpriteColors::White, "Viral");
			g_GameState->m_GameHelp->WriteLine(SpriteColors::White, "infestation");
			g_GameState->m_GameHelp->WriteLine(SpriteColors::White, "beware");
			g_GameState->m_GameHelp->WriteEnd();

			m_WaveInfo.learn_waveWon = false;
			m_WaveInfo.learn_levelWon = false;
		}
	}
	
	void GameRound::Classic_NextBadSector()
	{
		AssertIsclassic();
		
#if 1
		const int startLevel = 8;
		
		const int delta = m_WaveInfo.level - startLevel;
		
		if (delta < 0)
			return;
		if ((delta % 3) != 0)
			return;
#endif
		
		const Vec2F pos(Calc::Random((float)WORLD_SX), Calc::Random((float)WORLD_SY));
		
		g_World->SpawnBadSector(pos);
	}
	
	void GameRound::Classic_NextLevel()
	{
		AssertIsclassic();
		
		// update progress

		m_WaveInfo.level++;
		m_WaveInfo.levelType = Classic_GetLevelType();
		m_WaveInfo.wave = -1;
		m_WaveInfo.waveCount = Classic_GetWaveCount();
		m_WaveInfo.enemySpeedMultiplier = Classic_GetEnemySpeedMultiplier();
		m_WaveInfo.maxiHue = GetMaxiBossHue();
		
		// go from learning mode to play mode
		
		if (m_WaveInfo.level >= 1 && GameMode_get() == GameMode_ClassicLearn)
			m_GameMode = GameMode_ClassicPlay;

		if (m_GameMode == GameMode_ClassicLearn)
			m_WaveInfo.learn_levelWon = false;
		
		// setup level stats
		
//		if (m_WaveInfo.levelType == LevelType_Regular)
//		{
			if (GameMode_get() == GameMode_ClassicPlay)
			{
				switch (m_WaveInfo.difficulty)
				{
					case Difficulty_Easy:
					{
						m_WaveInfo.waveAliveTreshold = Calc::Min(6, 1 + m_WaveInfo.level);
						m_WaveInfo.waveMode = WaveMode_AliveCount;
						m_WaveInfo.bossCount = m_WaveInfo.level >= 2 ? 1 : 0;
						break;
					}
					case Difficulty_Hard:
					{
						m_WaveInfo.waveAliveTreshold = 0;
						m_WaveInfo.waveMode = WaveMode_Time;
						m_WaveInfo.bossCount = 2;
						m_WaveTimer.SetInterval(6.0f);
						m_WaveTimer.Start();
						break;
					}
					case Difficulty_Custom:
					{
						m_WaveInfo.waveAliveTreshold = 0;
						m_WaveInfo.waveMode = WaveMode_Time;
						m_WaveInfo.bossCount = 2;
						m_WaveTimer.SetInterval(6.0f);
						m_WaveTimer.Start();
						break;
					}
					case Difficulty_Unknown:
					{
#ifndef DEPLOYMENT
						throw ExceptionNA();
#else
						break;
#endif
					}
				}
			}
			else
			{
				m_WaveInfo.waveAliveTreshold = 0;
				m_WaveInfo.waveMode = WaveMode_AliveCount;
				m_WaveInfo.bossCount = 0;
			}
//		}
		
		if (m_WaveInfo.levelType == LevelType_EnemyAttack)
		{
			m_WaveInfo.waveMode = WaveMode_AliveCount;
			m_WaveInfo.bossCount = 0;
			if (m_WaveInfo.waveCount < 2)
				m_WaveInfo.waveCount = 2;
			g_GameState->m_SoundEffects->Play(Resources::SOUND_BOSS_ALARM_MINIBOSS, SfxFlag_MustFinish);
		}
		if (m_WaveInfo.levelType == LevelType_MiniAttack)
		{
			m_WaveInfo.waveMode = WaveMode_AliveCount;
			if (m_WaveInfo.difficulty == Difficulty_Easy)
				m_WaveInfo.bossCount = 5;
			else
				m_WaveInfo.bossCount = 7 + m_WaveInfo.level / 5;
			if (m_WaveInfo.waveCount < 2)
				m_WaveInfo.waveCount = 2;
			
			g_GameState->m_SoundEffects->Play(Resources::SOUND_BOSS_ALARM_MINIBOSS, SfxFlag_MustFinish);
		}
		
		// remove temporary powerup effects
		
		g_World->m_Player->RemoveTempPowerups();
		
		// update BGM

		Classic_UpdateBGM();

		// save game
		
		if (m_WaveInfo.levelWon)
		{
			if (m_WaveInfo.difficulty != Difficulty_Custom)
			{
				try
				{
					g_GameState->m_GameSave->Save();
				}
				catch (std::exception& e)
				{
					LOG_ERR("unable to save game: %s", e.what());
				}
			}
		}
		
		m_WaveInfo.levelWon = false;
	}
	
	void GameRound::Classic_UpdateBGM()
	{
		Res * oldBGM = g_GameState->GetMusic();
		Res * newBGM = g_GameState->m_ResMgr.Get(Classic_GetBGM(m_WaveInfo.level));

		if (newBGM != oldBGM)
		{
			g_GameState->PlayMusic(newBGM);
		}
	}

	void GameRound::ClassicLearn_IntroEnemy(EntityClass type)
	{
		Assert(m_WaveInfo.learn_waveWon == false);

		m_WaveInfo.learn_waveWon = true;
		m_WaveInfo.learn_nextEnemyType = type;
	}

	void GameRound::ClassicLearn_IntroBoss()
	{
		Assert(m_WaveInfo.learn_levelWon == false);

		m_WaveInfo.learn_waveWon = true;
		m_WaveInfo.learn_levelWon = true;
	}

	// ----------------------------------------
	// Intro Screen Game Mode
	// ----------------------------------------
	
	void GameRound::IntroScreen_WaveAdd(EnemyWave& wave)
	{
		AssertIsIntroScreen();
		
		m_EnemyWaveMgr.Add(wave);
	}
	
	// Themes
	
	void GameRound::RoundTheme_set(RoundTheme theme)
	{
		m_WaveInfo.theme = theme;
		
		switch (theme)
		{
			case RoundTheme_Fun:
				g_World->m_GridEffect->BaseHue_set(1.0f / 3.0f * 1.0f); // green
				break;
			case RoundTheme_HeavyAttack:
				g_World->m_GridEffect->BaseHue_set(1.0f / 6.0f * 1.0f); // yellow
				break;
			case RoundTheme_Currupt:
				g_World->m_GridEffect->BaseHue_set(1.0f / 6.0f * 5.0f); // purple
				break;
			case RoundTheme_Chillax:
				g_World->m_GridEffect->BaseHue_set(1.0f / 3.0f * 2.0f); // blue
				break;
			
#ifndef DEPLOYMENT
			default:
				throw ExceptionNA();
#else
			default:
				break;
#endif
		}
	}
	
	SpriteColor GameRound::RoundThemeColor_get() const
	{
		return m_WaveInfo.maxiColor;
	}
	
	// Operational
	
	LevelType GameRound::Classic_GetLevelType() const
	{
		AssertIsclassic();
		
		if (GameMode_get() == GameMode_ClassicLearn)
		{
			return LevelType_Regular;
		}
		else
		{
			RandomPicker<LevelType, LevelType__Count> picker;
								
			picker.Add(LevelType_Regular, 0.8f);
			
			if (m_WaveInfo.level >= 6)
				picker.Add(LevelType_MiniAttack, 0.1f);
			if (m_WaveInfo.level >= 7)
				picker.Add(LevelType_EnemyAttack, 0.1f);
						
			return picker.Get();
		}
	}
	
	int GameRound::Classic_GetWaveCount() const
	{	
		AssertIsclassic();
		
		if (GameMode_get() == GameMode_ClassicPlay)
		{
			switch (m_WaveInfo.difficulty)
			{
				case Difficulty_Easy:
					return 5 + m_WaveInfo.level / 6;
				case Difficulty_Hard:
					return 7 + m_WaveInfo.level / 3;
				case Difficulty_Custom:
					return 7 + m_WaveInfo.level / 3;
				default:
#ifndef DEPLOYMENT
				throw ExceptionNA();
#else
					return 1;
#endif
			}
		}
		else
		{
			return 10;
		}
	}
	
	RoundTheme GameRound::Classic_GetRoundTheme() const
	{
		AssertIsclassic();
		
		return (RoundTheme)((m_WaveInfo.level + m_WaveInfo.wave) % RoundTheme__Count);
	}
	
	static float Eval(const float* levelChances, size_t count, int level)
	{
		float result = -1.0f;
		
		for (size_t i = 0; i < count; ++i)
		{
			int begin = (int)levelChances[i * 2 + 0];
			float chance = levelChances[i * 2 + 1];
			
			if (chance == 0.0f)
				continue;
			
			if (level >= begin)
				result = chance;
		}
		
		return result;
	}
	
#define EVAL(levelChances, level) Eval(levelChances, sizeof(levelChances) / sizeof(float) / 2, level)
		
	EntityClass GameRound::Classic_GetEnemyType() const
	{
		AssertIsclassic();
		
#ifndef DEPLOYMENT
		if (Modifier_MakeLoveNotWar_get())
		{
#pragma message("hearts hack enabled")
			return EntityClass_Smiley;
		}
#endif
		if (m_WaveInfo.levelType == LevelType_Regular)
		{
			if (GameMode_get() == GameMode_ClassicLearn)
			{
				return m_WaveInfo.learn_nextEnemyType;
			}
			else
			{
				// decide which types are acceptable based on level
				
				RandomPicker<EntityClass, 20> picker;
				
				const float pi_kamikaze[] = 
				{
					2, 0.5f,
				};
				const float pi_shield[] =
				{
					3, 1.0f
				};			
				const float pi_triangle[] =
				{
					0, 1.0f
				};
				
				const float pi_triangle_biggy[] =
				{
					2, 0.5f
				};
				
				const float pi_triangle_extreme[] =
				{
					5, 0.5f
				};
				
				picker.Add(EntityClass_EvilTriangle, EVAL(pi_triangle, m_WaveInfo.level));
				picker.Add(EntityClass_EvilTriangleBiggy, EVAL(pi_triangle_biggy, m_WaveInfo.level));
				picker.Add(EntityClass_EvilTriangleExtreme, EVAL(pi_triangle_extreme, m_WaveInfo.level));
				picker.Add(EntityClass_Kamikaze, EVAL(pi_kamikaze, m_WaveInfo.level));
				picker.Add(EntityClass_Shield, EVAL(pi_shield, m_WaveInfo.level));
				
				// pick type

				return picker.Get();
			}
		}
		if (m_WaveInfo.levelType == LevelType_EnemyAttack)
		{
			RandomPicker<EntityClass, 20> picker;
			
			picker.Add(EntityClass_EvilTriangle, 1.0f);
			picker.Add(EntityClass_EvilTriangleBiggy, 0.4f);
			picker.Add(EntityClass_Kamikaze, 1.0f);
			
			return picker.Get();
		}
		if (m_WaveInfo.levelType == LevelType_MiniAttack)
		{
			return EntityClass_BorderPatrol;
		}
		
		return EntityClass_BorderPatrol;
	}
	
	SpawnFormation GameRound::Classic_GetEnemySpawnFormation(int count) const
	{
		AssertIsclassic();
		
		RandomPicker<SpawnFormation, SpawnFormation__Count> picker;
		
		for (int i = 0; i < SpawnFormation__Count; ++i)
		{
			SpawnFormation formation = (SpawnFormation)i;
			
			switch (GameMode_get())
			{
				case GameMode_ClassicPlay:
					if (formation == SpawnFormation_LearnMode_Drop)
						continue;
					if (m_WaveInfo.difficulty == Difficulty_Easy && m_WaveInfo.level <= 5 && formation == SpawnFormation_PlayerPos)
						continue;
					if (count < 6 && (formation == SpawnFormation_Line || formation == SpawnFormation_Prepared))
						continue;
//					picker.Add(formation, 1.0f);
					break;
				case GameMode_ClassicLearn:
					if (formation != SpawnFormation_LearnMode_Drop)
						continue;
					break;
				case GameMode_IntroScreen:
					if (formation == SpawnFormation_PlayerPos)
						continue;
					break;
				default:
#ifndef DEPLOYMENT
					throw ExceptionNA();
#else
					break;
#endif
			}
			
			picker.Add(formation, 1.0f);
		}
		
		return picker.Get();
	}
	
	int GameRound::Classic_GetEnemyCount() const
	{
		AssertIsclassic();
		
		switch (GameMode_get())
		{
			case GameMode_ClassicPlay:
			{
				switch (m_WaveInfo.difficulty)
				{
					case Difficulty_Easy:
						return Calc::Min(7, m_WaveInfo.level + 1);
					case Difficulty_Hard:
						return 9 + m_WaveInfo.level / 10;
					case Difficulty_Custom:
						return 9 + m_WaveInfo.level / 10;
					case Difficulty_Unknown:
#ifndef DEPLOYMENT
						throw ExceptionNA();
#else
						return 10;
#endif
				}
				break;
			}
			case GameMode_ClassicLearn:
			{
				return 2;
			}
			case GameMode_IntroScreen:
			{
				return 3;
			}
			default:
#ifndef DEPLOYMENT
				throw ExceptionNA();
#else
				break;
#endif
		}
		
		return 1;
	}
	
	BossType GameRound::Classic_GetMiniBossType() const
	{
		AssertIsclassic();
		
		// decide which types are acceptable based on level
		
		RandomPicker<BossType, BossType__Count> picker;
		
		picker.Add(BossType_Snake, 2.0f);
		
		if (m_WaveInfo.difficulty == Difficulty_Hard || m_WaveInfo.difficulty == Difficulty_Custom)
		{
			picker.Add(BossType_Spinner, 1.0f);
			picker.Add(BossType_Magnet, 1.0f);
		}
		if (m_WaveInfo.difficulty == Difficulty_Easy)
		{
			if (m_WaveInfo.level >= 3)
				picker.Add(BossType_Magnet, 1.0f);
			if (m_WaveInfo.level >= 6)
				picker.Add(BossType_Spinner, 1.0f);
		}
		
		// pick type
		
		return picker.Get();
	}
	
	int GameRound::Classic_GetMaxiBossType() const
	{
		AssertIsclassic();
		
		if (m_WaveInfo.difficulty == Difficulty_Easy)
			return Resources::BANDIT_EASY;
		if (m_WaveInfo.difficulty == Difficulty_Hard)
			return Resources::BANDIT_EASY;
		if (m_WaveInfo.difficulty == Difficulty_Custom)
		{
			if(g_GameState->m_GameSettings->m_CustomSettings.Custom_Boss)
				return Resources::BANDIT_HARD;
			else
				return Resources::BANDIT_EASY;
		}
		
		return Resources::BANDIT_EASY;
	}
	
	PowerupType GameRound::Classic_GetPowerupType() const
	{
		AssertIsclassic();
		
		if (m_WaveInfo.level == -1)
			return PowerupType_Credits;
		
		// decide which types are acceptable based on level
		
		RandomPicker<PowerupType, PowerupType__Count> picker;
		
		if (m_WaveInfo.level >= 0)
		{
			picker.Add(PowerupType_Credits, 1.0f);
		}
		
		if (m_WaveInfo.level >= 2)
		{
			picker.Add(PowerupType_Fun_Paddo, 0.1f);
			picker.Add(PowerupType_Fun_SlowMo, 0.1f);
			picker.Add(PowerupType_Fun_BeamFever, 0.3f);
			picker.Add(PowerupType_Health_ExtraLife, 0.1f);
			picker.Add(PowerupType_Health_Shield, 0.3f);
			picker.Add(PowerupType_Special_Max, 0.6f);
		}
		
		// pick type

		return picker.Get();
	}

	int GameRound::Classic_GetUpgradeCost(UpgradeType upgrade, int level) const
	{
		AssertIsclassic();
		
		return 1000;
	}
	
	float GameRound::Classic_GetEnemySpeedMultiplier() const
	{
		AssertIsclassic();
		
		if (m_GameMode == GameMode_ClassicPlay)
		{
			switch (m_WaveInfo.difficulty)
			{
				case Difficulty_Easy:
				{
					const int steps = 5;
					float baseSpeed = 0.3f;
					return Calc::Min(1.0f, baseSpeed + (1.0f - baseSpeed) * m_WaveInfo.level / steps);
				}
					
				case Difficulty_Hard:
					return 1.0f;
				case Difficulty_Custom:
					return 1.0f;
				
				default:
#ifndef DEPLOYMENT
					throw ExceptionNA();
#else
					return 1.0f;
#endif
			}
		}
		else
		{
			return 1.0f;
		}
	}
	
	int GameRound::Classic_GetBGM(int level) const
	{
#if defined(MACOS)
		return Resources::BGM_GAME1;
#elif defined(BBOS) || defined(WIN32) || defined(IPAD)
		int result = Resources::BGM_GAME3;
		switch (Modifier_Difficulty_get())
		{
			case Difficulty_Easy:
				if (level >= 6)
					result = Resources::BGM_GAME4;
				if (level >= 9)
					result = Resources::BGM_GAME5;
				break;
			case Difficulty_Hard:
			case Difficulty_Custom:
				if (level >= 4)
					result = Resources::BGM_GAME4;
				if (level >= 7)
					result = Resources::BGM_GAME5;
				break;
			default:
				Assert(false);
				break;
		}
		return result;
#else
		return Resources::BGM_GAME3;
#endif
	}

	float GameRound::GetTriangleHue()
	{
		m_TriangleHueTime.Increment(1.0321f);
		
		const float v1 = 0.12f;
		const float v2 = 0.81f;
		const float dv = v2 - v1;
		
		return fmodf(v1 + m_TriangleHueTimer.Progress_get() * dv, 1.0f);
	}
	
	float GameRound::GetMaxiBossHue() const
	{
		RandomPicker<float, 10> picker;
		
		picker.Add(0.0f, 1.0f);
		picker.Add(24.0f, 1.0f);
		picker.Add(65.0f, 1.0f);
		picker.Add(200.0f, 1.0f);
		picker.Add(265.0f, 1.0f);
		picker.Add(327.0f, 1.0f);
		
		return picker.Get() / 360.0f;
	}
	
	void GameRound::Spawn(EntityClass type, SpawnFormation formation, Vec2F pos, int count)
	{
		switch (formation)
		{
			case SpawnFormation_Circle:
			{
				float angle = Calc::Random(Calc::m2PI);
				//float arc = Calc::Random(0.2f, 1.0f) * Calc::m2PI * 2.0f;
				//count = (int)Calc::DivideUp(arc, 0.5f);
				float arc = count / 2.0f;
				float radius1 = 80.0f;
				float radius2 = 160.0f;
				
				EnemyWave wave;
				wave.MakeCircle(type, pos, radius1, radius2, angle, arc, count, SPAWN_INTERVAL);
				m_EnemyWaveMgr.Add(wave);
				break;
			}
			
			case SpawnFormation_Cluster:
			{
				EnemyWave wave;
				wave.MakeCluster(type, pos, 100.0f, count, SPAWN_INTERVAL);
				m_EnemyWaveMgr.Add(wave);
				break;
			}
			
			case SpawnFormation_LearnMode_Drop:
			{
				Vec2F position(0.0f, 100.0f);
				EnemyWave wave;
				wave.MakePrepared_Line(type, position, position, 1, m_WaveInfo.wave == 0 ? 0.0f : 1.0f, false, true, false, false);
				m_EnemyWaveMgr.Add(wave);
				break;
			}
			
			case SpawnFormation_Line:
			{
				float angle = Calc::Random(Calc::m2PI);
				float length = 100.0f;
				Vec2F dir = Vec2F::FromAngle(angle) * length;
				
				Vec2F position1 = pos - dir;
				Vec2F position2 = pos + dir;
				
				EnemyWave wave;
				wave.MakeLine(type, position1, position2, 20.0f, SPAWN_INTERVAL);
				m_EnemyWaveMgr.Add(wave);
				break;
			}
			
			case SpawnFormation_PlayerPos:
			{
				EnemyWave wave;
				wave.MakePlayerPos(type, count, SPAWN_INTERVAL);
				m_EnemyWaveMgr.Add(wave);
				break;
			}
			
			case SpawnFormation_Prepared:
			{
				Vec2F size(130.0f, 130.0f);
				float spacing = 130.0f;
				float angle = Calc::Random(4) * Calc::mPI2;
				
				Mat3x2 mat1;
				Mat3x2 mat2;
				
				mat1.MakeRotation(angle);
				mat2.MakeRotation(angle + Calc::mPI2);
				
				EnemyWave wave;
				wave.MakePrepared_Line(type, mat1 * Vec2F(spacing, -spacing), mat1 * Vec2F(spacing, +spacing), 5, 0.6f, true, true, true, true);
				m_EnemyWaveMgr.Add(wave);
				wave.MakePrepared_Line(type, mat2 * Vec2F(spacing, -spacing), mat2 * Vec2F(spacing, +spacing), 5, 0.6f, true, true, true, true);
				m_EnemyWaveMgr.Add(wave);
				
				break;
			}
			
#ifndef DEPLOYMENT
			default:
				throw ExceptionVA("unknown spawn formation");
#else
			default:
				break;
#endif
		}
	}
	
	void GameRound::ReplaceEnemiesByType(EntityClass srcType, EntityClass dstType)
	{
		// iterate through enemies
		
		for (int i = 0; i < g_World->m_enemies.PoolSize_get(); ++i)
		{
			EntityEnemy& enemy = g_World->m_enemies[i];
			
			if (!enemy.IsAlive_get())
				continue;
			if (enemy.Class_get() != srcType)
				continue;
			if (enemy.Class_get() == dstType)
				continue;
			
			enemy.Setup(dstType, enemy.Position_get(), EnemySpawnMode_Instant);
		}
	}
	
	bool GameRound::EnemyWaveMgrIsEmpty_get() const
	{
		bool result = true;
		
		result &= m_EnemyWaveMgr.IsEmpty_get();
		
		if (m_GameMode == GameMode_ClassicLearn)
			result &= m_WaveInfo.learn_waveWon == false;
		
		return result;
	}
	
	bool GameRound::Classic_GetSpawnMines() const
	{
		AssertIsclassic();
		
		return Calc::Random(10) == 0;
	}
	
	int GameRound::Classic_GetSpawnMineCount() const
	{
		AssertIsclassic();
		
		RandomPicker<int, 10> picker;
		if (m_WaveInfo.difficulty == Difficulty_Easy)
		{
			picker.Add(1, 1.0f);
			picker.Add(2, 0.2f);
		}
		else if (m_WaveInfo.difficulty == Difficulty_Hard)
		{
			picker.Add(1, 1.0f);
			picker.Add(2, 0.4f);
		}
		else if (m_WaveInfo.difficulty == Difficulty_Custom)
		{
			picker.Add(1, 1.0f);
			picker.Add(2, 0.4f);
		}
		else
		{
#ifndef DEPLOYMENT
			throw ExceptionNA();
#else
			picker.Add(1, 1.0f);
#endif
		}

		return picker.Get();
	}
	
	bool GameRound::Classic_GetSpawnRandomMines() const
	{
		AssertIsclassic();
		
		if (m_GameMode != GameMode_ClassicPlay)
			return false;

		bool result = true;
		
		result &= g_World->AliveMineCount_get() < Classic_GetMaxMineCount();
		
		if (m_WaveInfo.difficulty == Difficulty_Easy)
			result &= Calc::Random(2) == 0;

		return result;
	}
	
	int GameRound::Classic_GetSpawnRandomMineCount() const
	{
		AssertIsclassic();
		
		return 1;
	}
	
	int GameRound::Classic_GetMaxMineCount() const
	{
		AssertIsclassic();
		
		return 10;
	}
	
	bool GameRound::Classic_GetSpawnClutter() const
	{
		AssertIsclassic();
		
		if (m_GameMode != GameMode_ClassicPlay)
			return false;

		if(!g_GameState->m_GameSettings->m_CustomSettings.Clutter_Toggle)
			return false;

		return g_World->AliveClutterCount_get() < Classic_GetMaxClutterCount();
	}
	
	EntityClass GameRound::Classic_GetClutterType() const
	{
		AssertIsclassic();
		
		// decide which types are acceptable based on level
		
		RandomPicker<EntityClass, 20> picker;
		
		const float pi_square[] =
		{ 
			0, 1.0f
		};
		const float pi_square_biggy[] =
		{
			4, 1.0f
		};
		
		picker.Add(EntityClass_EvilSquare, EVAL(pi_square, m_WaveInfo.level));
		picker.Add(EntityClass_EvilSquareBiggy, EVAL(pi_square_biggy, m_WaveInfo.level));
		
		// pick type

		return picker.Get();
	}
	
	int GameRound::Classic_GetClutterCount() const
	{
		AssertIsclassic();
		
		if (m_WaveInfo.difficulty == Difficulty_Easy)
			return Calc::Min(6, 2 + m_WaveInfo.level / 5);
		if (m_WaveInfo.difficulty == Difficulty_Hard)
			return Calc::Min(9, 2 + m_WaveInfo.level / 3);
		if (m_WaveInfo.difficulty == Difficulty_Custom)
			return Calc::Min(9, 2 + m_WaveInfo.level / 3);
		
#ifndef DEPLOYMENT
		throw ExceptionNA();
#else
		return 0;
#endif
	}
	
	int GameRound::Classic_GetMaxClutterCount() const
	{
		AssertIsclassic();
		
		if (m_WaveInfo.difficulty == Difficulty_Easy)
			return 25;
		if (m_WaveInfo.difficulty == Difficulty_Hard)
			return 50;
		if (m_WaveInfo.difficulty == Difficulty_Custom)
		{
			if(g_GameState->m_GameSettings->m_CustomSettings.Clutter_Toggle)
				return 50;
			else
				return 0;
		}
		
#ifndef DEPLOYMENT
		throw ExceptionNA();
#else
		return 0;
#endif
	}
}
