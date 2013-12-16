#pragma once

namespace Game
{
	enum GameMode
	{
		GameMode_IntroScreen,
		GameMode_ClassicLearn,
		GameMode_ClassicPlay,
		GameMode_InvadersPlay
	};
	
	enum RoundState
	{
		RoundState_Idle,
		RoundState_PlayWaves,
		RoundState_PlayMaxiBoss,
		RoundState_LevelCleared
	};
	
	enum SpawnState
	{
		SpawnState_Idle, // nop
		SpawnState_CoolDown, // waiting for spawn
		SpawnState_Spawn, // spawning
		SpawnState_Wait, // waiting for finish
		SpawnState_Done // finished
	};
	
	enum WaveMode
	{
		WaveMode_AliveCount,
		WaveMode_Time
	};
	
	enum LevelType
	{
		LevelType_Regular, // enemies spawn in waves
		LevelType_EnemyAttack, // enemies attack from all sides (spawning outside the level border)
		LevelType_MiniAttack, // mini bosses attack from all sides
		LevelType__Count
	};
	
	enum RoundTheme
	{
		RoundTheme_Fun, // green, extra fun powerups, basic enemies
		RoundTheme_Chillax, // blue, powerups are energetic (beams), enemies basic
		RoundTheme_Currupt, // purple theme, powerups are poisoned, enemies are devious
		RoundTheme_HeavyAttack, // yellow (or black w/ night effect?), kamikazes abound, powerups geared towards strength
		RoundTheme__Count
	};
	
	enum UpgradeType
	{
		UpgradeType_Beam,
		UpgradeType_Shock,
		UpgradeType_Special,
		UpgradeType_Vulcan,
		UpgradeType_BorderPatrol,
		UpgradeType__Count
	};
	
	enum GameType
	{
		GameType_Classic,
		GameType_ClassicCustom
	};
}
