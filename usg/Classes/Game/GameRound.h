#pragma once

#include "BossBase.h"
#include "CallBack.h"
#include "EnemyWaveMgr.h"
#include "EntityPowerup.h"
#include "GameSettings.h"
#include "GameTypes.h"
#include "Log.h"
#include "PolledTimer.h"
#include "Types.h"

namespace Game
{
	enum GameModeFlag
	{
		GMF_IntroScreen = 1 << 0,
		GMF_Classic = 1 << 1,
		GMF_Invaders = 1 << 2
	};
	
	class WaveInfo
	{
	public:
		WaveInfo()
		{
			Initialize();
		}
		
		void Initialize()
		{
			difficulty = Difficulty_Easy;
			level = -1;
			wave = -1;
			levelType = LevelType_Regular;
			waveMode = WaveMode_AliveCount;
			waveAliveTreshold = 0;
			waveTime = 0;
			waveCount = 1;
			bossCount = 0;
			theme = RoundTheme_Currupt;
			enemySpeedMultiplier = 1.0f;
			levelWon = false;

			learn_waveWon = false;
			learn_levelWon = 0;
			learn_nextEnemyType = EntityClass_Undefined;
		}
		
		Difficulty difficulty;
		int level;
		int wave;
		LevelType levelType;
		WaveMode waveMode;
		int waveAliveTreshold;
		int waveTime;
		int waveCount;
		int bossCount;
		RoundTheme theme;
		float enemySpeedMultiplier;
		bool levelWon;
		float maxiHue;
		SpriteColor maxiColor;

		bool learn_waveWon;
		bool learn_levelWon;
		EntityClass learn_nextEnemyType;
	};
	
#define INVADERS_ENEMIES_SX 9
#define INVADERS_ENEMIES_SY 4
#define INVADERS_DEFENSE_SX 20
#define INVADERS_DEFENSE_SY 4
	
	enum InvadersState
	{
		InvadersState_Idle,
		InvadersState_Play,
		InvadersState_LevelWon,
	};
	
	class InvadersInfo
	{
	public:
		InvadersInfo()
		{
			Initialize();
		}
		
		void Initialize()
		{
			level = 0;
			state = InvadersState_Idle;
			wave = 0;

			InitializeLevel();
		}
		
		void InitializeLevel()
		{
			memset(offsetX, 0, sizeof(offsetX));
			offsetY = 0.0f;
			moveTimer = 0.0f;
			move = 0;
			memset(enemies, 0, sizeof(enemies));
			enemyCount = 0;
			memset(defense, 0, sizeof(defense));
		}
		
		int level;
		InvadersState state;
		int wave;
		float offsetX[INVADERS_ENEMIES_SY];
		float offsetY;
		float moveTimer;
		int move;
		Entity* enemies[INVADERS_ENEMIES_SY][INVADERS_ENEMIES_SX];
		int enemyCount;
		Entity* defense[INVADERS_DEFENSE_SY][INVADERS_DEFENSE_SX];
	};
	
	class GameRound
	{
	public:
		GameRound();
		~GameRound();
		void Initialize();
		
		// Shared between all game modes
		void Clear();
		void Setup(GameMode mode, Difficulty difficulty);
		void Update(float dt);
		void RenderAdditive();
		
		GameMode GameMode_get() const { return m_GameMode; }
		bool GameModeTest(int mask)
		{
			bool result = false;
			if (mask & GMF_Classic)
				result |= GameModeIsClassic();
			if (mask & GMF_IntroScreen)
				result |= GameModeIsIntroScreen();
			if (mask & GMF_Invaders)
				result |= GameModeIsInvaders();
			return result;
		}
		bool GameModeIsIntroScreen() const { return m_GameMode == GameMode_IntroScreen; }
		bool GameModeIsClassic() const { return m_GameMode == GameMode_ClassicLearn || m_GameMode == GameMode_ClassicPlay; }
		bool GameModeIsInvaders() const { return m_GameMode == GameMode_InvadersPlay; }
		
		Difficulty Modifier_Difficulty_get() const { return m_WaveInfo.difficulty; }
		float Modifier_EnemySpeedMultiplier_get() const { return m_WaveInfo.enemySpeedMultiplier; }
		float Modifier_PlayerDamageMultiplier_get() const { return m_WaveInfo.difficulty == Difficulty_Hard ? 1.0f / 0.66f : 1.0f; }
		bool Modifier_MakeLoveNotWar_get() const { return m_LoveNotWar; }
		
		// Theme
		
		void RoundTheme_set(RoundTheme theme); // basically just used to cycle through various background colors
		SpriteColor RoundThemeColor_get() const; // returns the background base color
		float BackgroundHue_get() const { return m_WaveInfo.maxiHue; } // returns the background base hue
		
		RoundTheme m_RoundTheme;

		// Hacks to make various stuff work from the age all was public and known ..
		void LOAD_Difficulty_set(Difficulty difficulty) { m_WaveInfo.difficulty = difficulty; }
		void LOAD_Level_set(int level) { m_WaveInfo.level = level; }
		void DEBUG_LevelOffset(int offset) { m_WaveInfo.level += offset; }
		
		// ----------------------------------------
		// Classic game mode
		// ----------------------------------------
		
		void Classic_Update(float dt);
		
		// Classic Game Mode: Events
		CallBack Classic_OnWaveBegin;
		CallBack Classic_OnMiniBossBegin;
		CallBack Classic_OnMaxiBossBegin;
		CallBack Classic_OnLevelCleared;
		
		// Classic Game Mode: Public Interface
		void       Classic_Level_set(int level);
		int        Classic_Level_get() const;
		int        Classic_Wave_get() const;
		int        Classic_WaveCount_get() const;
		void       Classic_WaveAdd(EnemyWave& wave);
		RoundState Classic_RoundState_get() const { return m_RoundState; }
		
		// Classic Game Mode: Spawnage
		void Classic_NextLevel();
		void Classic_NextWave();
		void Classic_NextMiniBoss();
		void Classic_NextMaxiBoss();
		void Classic_NextBadSector();
		void Classic_UpdateBGM();
		void ClassicLearn_IntroEnemy(EntityClass type);
		void ClassicLearn_IntroBoss();
		
		// Classic Game Mode: Events
		void Classic_HandleLevelBegin();
		void Classic_HandleLevelWin();
		void Classic_HandleWaveBegin();
		void Classic_HandleWaveWinAll();
//		void Classic_HandleMiniBossBegin();
//		void Classic_HandleMiniBossWin();
		void Classic_HandleMaxiBossBegin();
		void Classic_HandleMaxiBossWin();
		void Classic_HandleHelpComplete();
		
		// Classic Game Mode: Event Triggers
		bool Classic_HasWonLevel() const;
		bool Classic_HasWonWave() const;
		bool Classic_HasReachedWaveTreshold() const;
		bool Classic_HasReachedWaveFinal() const;
		bool Classic_HasWonWaveAll() const;
		bool Classic_HasWonMaxiBoss() const;
		
		// Classic Game Mode: State Transition
		void Classic_RoundState_set(RoundState state);
		void Classic_WaveSpawnState_set(SpawnState state);
		void Classic_MaxiSpawnState_set(SpawnState state);
		
		// Classic Game Mode: Operational Details
		LevelType      Classic_GetLevelType() const;
		int            Classic_GetWaveCount() const;
		RoundTheme     Classic_GetRoundTheme() const;
		EntityClass    Classic_GetEnemyType() const;
		SpawnFormation Classic_GetEnemySpawnFormation(int count) const;
		int            Classic_GetEnemyCount() const;
		BossType       Classic_GetMiniBossType() const;
		int            Classic_GetMaxiBossType() const;
		PowerupType    Classic_GetPowerupType() const;
		Vec2F          Classic_GetDeploymentLocation() const; // select deployment location for drop boxes
		int            Classic_GetUpgradeCost(UpgradeType upgrade, int level) const;
		float          Classic_GetEnemySpeedMultiplier() const;
		int            Classic_GetBGM(int level) const;
		
		// Classic Game Mode: Operational: Waves
		bool        Classic_GetSpawnMines() const;
		int         Classic_GetSpawnMineCount() const;
		bool        Classic_GetSpawnRandomMines() const;
		int         Classic_GetSpawnRandomMineCount() const;
		int         Classic_GetMaxMineCount() const;
		bool        Classic_GetSpawnClutter() const;
		EntityClass Classic_GetClutterType() const;
		int         Classic_GetClutterCount() const;
		int         Classic_GetMaxClutterCount() const;
		
		inline void AssertIsclassic() const
		{
			Assert(m_GameMode == GameMode_ClassicLearn || m_GameMode == GameMode_ClassicPlay);
		}
		
		// ----------------------------------------
		// Intro screen game mode
		// ----------------------------------------
		void        IntroScreen_Update(float dt);
		void        IntroScreen_WaveAdd(EnemyWave& wave);
		EntityClass IntroScreen_GetEnemyType() const;
		
		inline void AssertIsIntroScreen() const
		{
			Assert(m_GameMode == GameMode_IntroScreen);
		}
		
		// ----------------------------------------
		// Invaders Game Mode
		// ----------------------------------------
		void Invaders_Update(float dt);
		void Invaders_UpdateEnemyPositions(float dt);
        void Invaders_HandleEnemyDeath(Entity* enemy);
		Vec2F Invaders_GetEnemyPos(int x, int y);
		
		// Powerups
		void Invaders_DefenseRestore(); // the defense is rebuilt
		void Invaders_DefenseHarden(); // defense goes up in strength
		
		// Events
		void Invaders_HandleLevelWin(); // invoked when the level has been completed
		
		// Event triggers
		bool Invaders_CheckLevelWin(); // returns true if the level has been completed. eg, all enemies have been beat
		
		// Spawnage
		void Invaders_NextLevel(); // begin an entirely new level, filled with enemies
		void Invaders_NextUfo(); // spawn a UFO, with abduction beam!
		
		// Stochastics
		bool Invaders_GetSpawnUfo(); // returns true if we should randomly spawn a UFO
		EntityClass Invaders_GetEnemyType(int row); // returns a randomized type of enemy for the given row
		bool Invaders_GetEnemyFire(); // randomly returns true to set the firing rate for enemies
		float Invaders_GetEnemyFireDamage();
		float Invaders_GetEnemyFireSpeed();
		
		inline void AssertIsInvaders() const
		{
			Assert(m_GameMode == GameMode_InvadersPlay);
		}
		
		InvadersInfo m_InvadersInfo;
		
		// General operational stuff:
		float GetTriangleHue();
		float GetMaxiBossHue() const;
		void Spawn(EntityClass type, SpawnFormation formation, Vec2F pos, int count);
		void ReplaceEnemiesByType(EntityClass srcType, EntityClass dstType);
		bool EnemyWaveMgrIsEmpty_get() const;
		
	private:
		GameMode m_GameMode;
		GameType m_GameType; // used to diff between custom and classic games
		
		EnemyWaveMgr m_EnemyWaveMgr;
		
		TimeTracker m_TriangleHueTime;
		AnimTimer m_TriangleHueTimer;

		bool m_LoveNotWar;
		
		LogCtx m_Log;
		
		// Classic Game Mode
		RoundState m_RoundState;
		SpawnState m_WaveSpawnState;
		SpawnState m_MaxiSpawnState;
		WaveInfo m_WaveInfo;
		int m_CustomWaveCount;
		
		PolledTimer m_WaveTimer; // used when wave start time driven
		PolledTimer m_RoundCoolDownTimer; // round warmup timer, allows player to spawn, and have some rest at the beginning of each round
		PolledTimer m_WaveCoolDownTimer; // used when wave start treshold driven
		PolledTimer m_MaxiCoolDownTimer; // used to spawn maxi boss
		PolledTimer m_LevelCoolDownTimer;
	};
}
