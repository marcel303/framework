#pragma once

#include "FixedSizeString.h"

#ifdef DEPLOYMENT
	#define SCREENSHOT_MODE 0
#else
	#define SCREENSHOT_MODE 0
#endif

extern int VIEW_SX;
extern int VIEW_SY;
extern int SCREEN_SX;
extern int SCREEN_SY;

#if defined(BBOS)
#define WORLD_SX (1024/2*3)
#define WORLD_SY (600/2*3)
#else
#define WORLD_SX (480 * 3)
#define WORLD_SY (320 * 3)
#endif

#define WORLD_S Vec2F(WORLD_SX, WORLD_SY)

#define WORLD_MIDX (WORLD_SX / 2)
#define WORLD_MIDY (WORLD_SY / 2)

#define WORLD_MID Vec2F(WORLD_MIDX, WORLD_MIDY)

#define MAXHITLIST 10

// --------------------
//pool allocations
// --------------------
#define SHIP_POOL_SIZE 200
#define BULLET_POOL_SIZE 350

#define BULLET_DEFAULT_SPEED 300.0f // (300 / 60 = 5 pixels/update) todo: use line tracing on bullet hit test? -> would allow for even faster movement..
#define BULLET_DEFAULT_LIFE 20.0f //Bullet lifetime in seconds

// --------------------
// player
// --------------------
#define PLAYER_SPEED 200.0f

// --------------------
// enemies
// --------------------
#define ENEMY_DEFAULT_COLLISION_SIZE 8.0f			//Temporary size
#define ENEMY_DEFAULT_AVOID_SIZE 16.0f
#define ENEMY_DEFAULT_SPEED (PLAYER_SPEED * 0.9f)
#define ENEMY_KAMIKAZE_BOOST_RANGE 80.0f

// --------------------
// camera
// --------------------
#define ZOOM_PIXELS 300

// --------------------
// mini bosses
// --------------------
#define SPINNER_SEGMENT_HEALTH 10.0f
#define SNAKE_SEGMENT_HEALTH 15.0f

// --------------------
// player
// --------------------
#define MAX_NAME_LENGTH 12

//

#define DIFFICULTYMOD 1
#define LEVELMOD 1

//HUD/INTERFACE

#define TOUCH_OFFSET 30.0f

enum Difficulty
{
	Difficulty_Easy = 0,
	Difficulty_Hard = 1,
	Difficulty_Custom = 2,
	Difficulty_Unknown = 99
};

enum ControllerType
{
	ControllerType_Undefined = -1,
	ControllerType_Standard = 0,
	ControllerType_DualAnalog = 1,
	ControllerType_Tilt = 2,
	ControllerType_Gamepad = 3,
	ControllerType_Keyboard = 4,
	ControllerType_Gamepad_Psp = 5,
};

#include "Types.h"

namespace Game
{
	enum GameSetting
	{
		GameSetting_SoundEnabled = 1 << 0,
		GameSetting_SoundVolume = 1 << 1,
		GameSetting_MusicEnabled = 1 << 2,
		GameSetting_MusicVolume = 1 << 3,
		GameSetting_ControllerType = 1 << 4,
		GameSetting_ControllerModeMove = 1 << 5,
		GameSetting_ControllerModeFire = 1 << 6,
		GameSetting_HudMode = 1 << 7,
		GameSetting_HudOpacity = 1 << 8,
		GameSetting_CameraSemi3dEnabled = 1 << 9
	};
	
	enum ControllerMode
	{
		ControllerMode_AutoCalibrate = 0,
		ControllerMode_Direct = 1,
		ControllerMode__Count
	};
	
	enum HudMode
	{
		HudMode_Standard = 0,
		HudMode_Compact = 1,
		HudMode__Count
	};

	//v1.3

	class CustomSettings
	{
	public:
		bool Boss_Toggle;
		bool Custom_Boss;
		bool Wave_Toggle;
		bool Mines_Toggle;
		bool Clutter_Toggle;
		bool PowerUp_Toggle;
		bool UpgradesUnlock_Toggle;
		bool Invuln_Toggle;
		bool InfLives_Toggle;
		int StartLevel;
	};

	class GameSettings
	{
	public:
		GameSettings();
		~GameSettings();
		void Initialize();
		
		void Save();
		void Load();
		void Apply();
		void ApplyFiltered(int mask);
		
		void SetSoundEnabled(bool enabled);
		void ToggleSoundEnabled();
		void SetSoundVolume(float volume);
		
		void SetMusicEnabled(bool enabled);
		void ToggleMusicEnabled();
		void SetMusicVolume(float volume);
		
		void SwitchControllerType();
		void SetControllerType(ControllerType type);
		
		void SetControllerModeMove(ControllerMode mode);
		void SetControllerModeFire(ControllerMode mode);
		void SetHudMode(HudMode mode);
		void SetHudOpacity(float opacity);
		void SetCameraSemi3dEnabled(bool enabled);

		void ToggleBossEnabled();
		void ToggleCustomBoss();
		void ToggleWavesEnabled();
		void ToggleMinesEnabled();
		void ToggleClutterEnabled();
		void TogglePowerUpEnabled();
		void ToggleUpgradesUnlocked();
		void ToggleInvuln();
		void ToggleInfiniteLives();

		void ResetCustomSettings();

		void AdjustLvl(int val);
		
		bool m_SoundEnabled;
		float m_SoundVolume;
		
		bool m_MusicEnabled;
		float m_MusicVolume;
		
		ControllerType m_ControllerType;
		ControllerMode m_ControllerModeMove;
		ControllerMode m_ControllerModeFire;

		HudMode m_HudMode;
		float m_HudOpacity;
		
		int m_ScoreHistory;
		
		bool m_ScreenFlip;
		bool m_CameraSemi3dEnabled;

		// v1.3

		CustomSettings m_CustomSettings;
		
		// dynamic
		
		int m_StartupCount;
		FixedSizeString<MAX_NAME_LENGTH> m_PlayerName;
		bool m_PlayTutorial;

#if defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(BBOS)
		int m_Button_UseShockwave;
		int m_Button_UseSpecial;
		int m_Axis_FireX;
		int m_Axis_FireY;
		int m_Axis_MoveX;
		int m_Axis_MoveY;
		int m_Button_NavigateL;
		int m_Button_NavigateR;
		int m_Button_NavigateU;
		int m_Button_NavigateD;
		int m_Button_SwitchWeapons;
		int m_Button_MenuButtonSelect;
		int m_Button_Dismiss;
		int m_Button_UpgradeMenu;
		int m_Button_Select;
#endif

		// 2.0

		int m_TiltControl_CalibrationAngle;
	};
	
	extern const char* DifficultyToString();
}

#if defined(PSP)
#define PSPSAVE_APPNAME   "CRITWAVE"
#define PSPSAVE_DESC      "Critical Wave"
#define PSPSAVE_DESC_LONG "High scores, settings and auto-save"
#define PSPSAVE_SCORES    "SAVEHIGH.BIN"
#define PSPSAVE_SETTINGS  "SAVECONF.BIN"
#define PSPSAVE_CONTINUE  "SAVEGAME.BIN"
#endif
