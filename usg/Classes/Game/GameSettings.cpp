#include "ConfigState.h"
#include "EntityPlayer.h"
#include "GameRound.h"
#include "GameSettings.h"
#include "GameState.h"
#include "ISoundEffectMgr.h"
#include "ISoundPlayer.h"
#include "Menu_InGame.h"
#include "MenuMgr.h"
#include "PlayerController.h"
#ifdef WIN32
#include "PlayerController_Gamepad.h"
#include "PlayerController_Keyboard.h"
#endif
#ifdef LINUX
#include "PlayerController_Gamepad.h"
#include "PlayerController_Keyboard.h"
#endif
#ifdef MACOS
#include "PlayerController_Gamepad.h"
#include "PlayerController_Keyboard.h"
#endif
#ifdef PSP
#include "PlayerController_Gamepad_Psp.h"
#endif
#ifdef BBOS
#include "Gamepad.h"
#include "PlayerController_Gamepad.h"
#endif
#include "System.h"
#include "World.h"

#define SAVE_PATH g_System.GetDocumentPath("config.dat")

#if defined(IPAD)
//int VIEW_SX = 1024;
//int VIEW_SY = 768;
int VIEW_SX = 480;
int VIEW_SY = 360;
#elif defined(PSP_UI)
int VIEW_SX = 480;
int VIEW_SY = 272;
#elif defined(BBOS_ALLTOUCH)
int VIEW_SX = 1280/2.4; // divide BlackBerry AllTouch screen size by 2.
int VIEW_SY = 768/2.4;
int SCREEN_SX = 1280;
int SCREEN_SY = 768;
#elif defined(BBOS)
int VIEW_SX = 1024/2; // divide BlackBerry PlayBook screen size by 2.
int VIEW_SY = 600/2;
int SCREEN_SX = 1024;
int SCREEN_SY = 600;
#else
// BlackBerry AllTouch
int VIEW_SX = 1280/2.4;
int VIEW_SY = 768/2.4;

// iPhone
//int VIEW_SX = 480;
//int VIEW_SY = 320;

// PlayStation Portable
//int VIEW_SX = 480;
//int VIEW_SY = 272;
#endif

namespace Game
{
	GameSettings::GameSettings()
	{
		Initialize();
	}
	
	GameSettings::~GameSettings()
	{
	}
	
	void GameSettings::Initialize()
	{
		m_SoundEnabled = XTRUE;
		m_SoundVolume = 1.0f;
		
		m_MusicEnabled = XTRUE;
		m_MusicVolume = 1.0f;
		
#if defined(WIN32) || defined(LINUX) || defined(MACOS) || defined(BBOS) || defined(IPHONEOS)
		m_ControllerType = ControllerType_DualAnalog;
#elif defined(PSP)
		m_ControllerType = ControllerType_Gamepad_Psp;
#else
		m_ControllerType = ControllerType_Tilt;
#endif

#if defined(IPHONEOS) || defined(BBOS)
		m_ControllerModeMove = ControllerMode_Direct;
		m_ControllerModeFire = ControllerMode_Direct;
#else
		m_ControllerModeMove = ControllerMode_AutoCalibrate;
		m_ControllerModeFire = ControllerMode_AutoCalibrate;
#endif

		m_HudMode = HudMode_Standard;
		m_HudOpacity = 1.0f;
			
#if defined(IPHONEOS)
		m_ScoreHistory = 7; // note: iOS integration only supports 0 (all time) or 7
#else
		m_ScoreHistory = 7;
#endif
		
		m_CameraSemi3dEnabled = false;
		
		m_ScreenFlip = false;
		
		m_StartupCount = 0;

		m_PlayTutorial = true;

		// v1.3
		ResetCustomSettings();

#if defined(WIN32) || defined(LINUX) || defined(MACOS)
		m_Button_UseShockwave = -1;
		m_Button_UseSpecial = -1;
		m_Axis_FireX = -1;
		m_Axis_FireY = -1;
		m_Axis_MoveX = -1;
		m_Axis_MoveY = -1;
		m_Button_NavigateL = -1;
		m_Button_NavigateR = -1;
		m_Button_NavigateU = -1;
		m_Button_NavigateD = -1;
		m_Button_SwitchWeapons = -1;
		m_Button_MenuButtonSelect = -1;
		m_Button_Dismiss = -1;
		m_Button_UpgradeMenu = -1;
		m_Button_Select = -1;
#elif defined(BBOS)
		m_Button_UseShockwave = GamepadButton_Left_Trigger;
		m_Button_UseSpecial = GamepadButton_Right_Trigger;
		m_Axis_FireX = GamepadAnalog_RightX;
		m_Axis_FireY = GamepadAnalog_RightY;
		m_Axis_MoveX = GamepadAnalog_LeftX;
		m_Axis_MoveY = GamepadAnalog_LeftY;
		m_Button_NavigateL = -1;
		m_Button_NavigateR = -1;
		m_Button_NavigateU = -1;
		m_Button_NavigateD = -1;
		m_Button_SwitchWeapons = GamepadButton_Right_Right;
		m_Button_MenuButtonSelect = -1;
		m_Button_Dismiss = -1;
		m_Button_UpgradeMenu = GamepadButton_Right_Up;
		m_Button_Select = -1;
#endif

		// 2.0

		m_TiltControl_CalibrationAngle = 35;
	}

	void GameSettings::Save()
	{
		try
		{
			ConfigSetInt("sound.enabled", m_SoundEnabled ? 1 : 0);
			ConfigSetInt("sound.volume", (int)(m_SoundVolume * 1024.0f));
			
			ConfigSetInt("music.enabled", m_MusicEnabled ? 1 : 0);
			ConfigSetInt("music.volume", (int)(m_MusicVolume * 1024.0f));

			ConfigSetInt("hud.mode", (int)m_HudMode);
			ConfigSetInt("hud.opacity", (int)(m_HudOpacity * 1024.0f));
			ConfigSetInt("camera.semi3d_enabled", m_CameraSemi3dEnabled ? 1 : 0);
			
			ConfigSetInt("controller.type", (int)m_ControllerType);
			ConfigSetInt("controller.move_mode", (int)m_ControllerModeMove);
			ConfigSetInt("controller.fire_mode", (int)m_ControllerModeFire);
			
			ConfigSetInt("app.start_count", m_StartupCount);
			ConfigSetInt("app.screen_flip", m_ScreenFlip ? 1 : 0);

			ConfigSetString("player.name", m_PlayerName.c_str());
			ConfigSetInt("player.tutorial", m_PlayTutorial ? 1 : 0);

#if defined(WIN32) || defined(LINUX) || defined(MACOS)
			ConfigSetInt("gamepad.use_shockwave", m_Button_UseShockwave);
			ConfigSetInt("gamepad.use_special", m_Button_UseSpecial);
			ConfigSetInt("gamepad.move_x", m_Axis_MoveX);
			ConfigSetInt("gamepad.move_y", m_Axis_MoveY);
			ConfigSetInt("gamepad.fire_x", m_Axis_FireX);
			ConfigSetInt("gamepad.fire_y", m_Axis_FireY);
			ConfigSetInt("gamepad.navigate_left", m_Button_NavigateL);
			ConfigSetInt("gamepad.navigate_right", m_Button_NavigateR);
			ConfigSetInt("gamepad.navigate_up", m_Button_NavigateU);
			ConfigSetInt("gamepad.navigate_down", m_Button_NavigateD);
			ConfigSetInt("gamepad.weapon_switch", m_Button_SwitchWeapons);
			ConfigSetInt("gamepad.menu_select", m_Button_MenuButtonSelect);
			ConfigSetInt("gamepad.dismiss", m_Button_Dismiss);
			ConfigSetInt("gamepad.menu_upgrade", m_Button_UpgradeMenu);
			ConfigSetInt("gamepad.select", m_Button_Select);
#endif

			ConfigSetInt("tilt.angle", m_TiltControl_CalibrationAngle);

			ConfigSave(false);
		}
		catch (std::exception& e)
		{
			LOG_ERR("unable to save settings: %s", e.what());
		}
	}
	
	void GameSettings::Load()
	{
		try
		{
			m_SoundEnabled = ConfigGetIntEx("sound.enabled", m_SoundEnabled ? 1 : 0) != 0;
			m_SoundVolume = ConfigGetIntEx("sound.volume", (int)(m_SoundVolume * 1024.0f)) / 1024.0f;
			
			m_MusicEnabled = ConfigGetIntEx("music.enabled", m_MusicEnabled ? 1 : 0) != 0;
			m_MusicVolume = ConfigGetIntEx("music.volume", (int)(m_MusicVolume * 1024.0f)) / 1024.0f;

			m_HudMode = (HudMode)ConfigGetIntEx("hud.mode", (int)m_HudMode);
			m_HudOpacity = ConfigGetIntEx("hud.opacity", (int)(m_HudOpacity * 1024.0f)) / 1024.0f;
			m_ScoreHistory = ConfigGetIntEx("hud.score_history", m_ScoreHistory);
			m_CameraSemi3dEnabled = ConfigGetIntEx("camera.semi3d_enabled", m_CameraSemi3dEnabled ? 1 : 0) != 0;
			
#if !defined(BBOS)
			m_ControllerType = (ControllerType)ConfigGetIntEx("controller.type", (int)m_ControllerType);
#endif
			m_ControllerModeMove = (ControllerMode)ConfigGetIntEx("controller.move_mode", (int)m_ControllerModeMove);
			m_ControllerModeFire = (ControllerMode)ConfigGetIntEx("controller.fire_mode", (int)m_ControllerModeFire);
			
			m_StartupCount = ConfigGetIntEx("app.start_count", m_StartupCount);
#if defined(IPONEOS)
			m_ScreenFlip = ConfigGetIntEx("app.screen_flip", m_ScreenFlip ? 1 : 0) != 0;
#else
			m_ScreenFlip = false;
#endif

			m_PlayerName = ConfigGetStringEx("player.name", m_PlayerName.c_str()).c_str();
			m_PlayTutorial = ConfigGetIntEx("player.tutorial", m_PlayTutorial ? 1 : 0) != 0;

#if defined(WIN32) || defined(LINUX) || defined(MACOS)
			m_Button_UseShockwave = ConfigGetIntEx("gamepad.use_shockwave", m_Button_UseShockwave);
			m_Button_UseSpecial = ConfigGetIntEx("gamepad.use_special", m_Button_UseSpecial);
			m_Axis_MoveX = ConfigGetIntEx("gamepad.move_x", m_Axis_MoveX);
			m_Axis_MoveY = ConfigGetIntEx("gamepad.move_y", m_Axis_MoveY);
			m_Axis_FireX = ConfigGetIntEx("gamepad.fire_x", m_Axis_FireX);
			m_Axis_FireY = ConfigGetIntEx("gamepad.fire_y", m_Axis_FireY);
			m_Button_NavigateL = ConfigGetIntEx("gamepad.navigate_left", m_Button_NavigateL);
			m_Button_NavigateR = ConfigGetIntEx("gamepad.navigate_right", m_Button_NavigateR);
			m_Button_NavigateU = ConfigGetIntEx("gamepad.navigate_up", m_Button_NavigateU);
			m_Button_NavigateD = ConfigGetIntEx("gamepad.navigate_down", m_Button_NavigateD);
			m_Button_SwitchWeapons = ConfigGetIntEx("gamepad.weapon_switch", m_Button_SwitchWeapons);
			m_Button_MenuButtonSelect = ConfigGetIntEx("gamepad.menu_select", m_Button_MenuButtonSelect);
			m_Button_Dismiss = ConfigGetIntEx("gamepad.dismiss", m_Button_Dismiss);
			m_Button_UpgradeMenu = ConfigGetIntEx("gamepad.menu_upgrade", m_Button_UpgradeMenu);
			m_Button_Select = ConfigGetIntEx("gamepad.select", m_Button_Select);
#endif

			m_TiltControl_CalibrationAngle = ConfigGetIntEx("tilt.angle", m_TiltControl_CalibrationAngle);
		}
		catch (std::exception& e)
		{
			LOG_ERR("unable to load settings: %s", e.what());
		}
	}
	
	void GameSettings::Apply()
	{
		ApplyFiltered(0xFFFFFFFF);
	}
	
	void GameSettings::ApplyFiltered(int mask)
	{
		try
		{
			if (mask & GameSetting_SoundEnabled)
			{
				g_GameState->m_SoundEffectMgr->IsEnabled_set(m_SoundEnabled);
			}
			
			if (mask & GameSetting_SoundVolume)
			{
				g_GameState->m_SoundEffectMgr->Volume_set(m_SoundVolume);
			}
			
			if (mask & GameSetting_MusicEnabled)
			{
				g_GameState->m_SoundPlayer->IsEnabled_set(m_MusicEnabled);
			}
			
			if (mask & GameSetting_MusicVolume)
			{
				g_GameState->m_SoundPlayer->Volume_set(m_MusicVolume);
			}
			
			if (mask & GameSetting_ControllerType)
			{
				g_World->PlayerController_set(0);

				IPlayerController* controller = 0;
			
#if defined(BBOS)
				switch (m_ControllerType)
				{
				case ControllerType_DualAnalog:
					controller = new PlayerController_DualAnalog();
					break;
				case ControllerType_Gamepad:
					controller = new PlayerController_Gamepad();
					break;
				default:
					Assert(false);
					controller = new PlayerController_DualAnalog();
					break;
				}
#elif defined(IPHONEOS)
				switch (m_ControllerType)
				{
				case ControllerType_DualAnalog:
					controller = new PlayerController_DualAnalog();
					break;
				//case ControllerType_Gamepad:
				//	controller = new PlayerController_Gamepad();
				//	break;
				case ControllerType_Tilt:
					controller = new PlayerController_Tilt();
					break;
				default:
					Assert(false);
					controller = new PlayerController_DualAnalog();
					break;
				}
#elif defined(WIN32) || defined(LINUX) || defined(MACOS)
				switch (m_ControllerType)
				{
				case ControllerType_DualAnalog:
					controller = new PlayerController_DualAnalog();
					break;
				case ControllerType_Gamepad:
					controller = new PlayerController_Gamepad();
					break;
				case ControllerType_Keyboard:
					controller = new PlayerController_Keyboard();
					break;
				default:
					Assert(false);
					controller = new PlayerController_Keyboard();
					break;
				}
#elif defined(PSP)
				switch (m_ControllerType)
				{
				case ControllerType_Gamepad_Psp:
					controller = new PlayerController_Gamepad_Psp();
					break;
				default:
					Assert(false);
					controller = new PlayerController_Gamepad_Psp();
					break;
				}
#else
	#error
#endif

				Assert(controller != 0);

				g_World->PlayerController_set(controller);
			}
			
			if (mask & GameSetting_ControllerModeMove)
			{
				// nop
			}
			
			if (mask & GameSetting_ControllerModeFire)
			{
				// nop
			}
			
			if (mask & GameSetting_HudMode)
			{
				GameMenu::Menu_InGame* menu = (GameMenu::Menu_InGame*)g_GameState->Interface_get()->Menu_get(GameMenu::MenuType_InGame);
				
				menu->HudMode_set(m_HudMode);
			}
			
			if (mask & GameSetting_HudOpacity)
			{
				GameMenu::Menu_InGame* menu = (GameMenu::Menu_InGame*)g_GameState->Interface_get()->Menu_get(GameMenu::MenuType_InGame);
				
				menu->HudOpacity_set(m_HudOpacity);
			}
			
			if (mask & GameSetting_CameraSemi3dEnabled)
			{
				// nop
			}
		}
		catch (std::exception& e)
		{
			LOG_ERR("unable to apply settings: %s", e.what());
		}
	}
	
	void GameSettings::SetSoundEnabled(bool enabled)
	{
		m_SoundEnabled = enabled;
		
		ApplyFiltered(GameSetting_SoundEnabled);
	}
	
	void GameSettings::ToggleSoundEnabled()
	{
		SetSoundEnabled(!m_SoundEnabled);
	}
	
	void GameSettings::SetSoundVolume(float volume)
	{
		m_SoundVolume = volume;
		
		ApplyFiltered(GameSetting_SoundVolume);
	}
	
	void GameSettings::SetMusicEnabled(bool enabled)
	{
		m_MusicEnabled = enabled;
		
		ApplyFiltered(GameSetting_MusicEnabled);
	}
	
	void GameSettings::ToggleMusicEnabled()
	{
		SetMusicEnabled(!m_MusicEnabled);
	}
	
	void GameSettings::SetMusicVolume(float volume)
	{
		m_MusicVolume = volume;
		
		ApplyFiltered(GameSetting_MusicVolume);
	}
	
	void GameSettings::SwitchControllerType()
	{
		// todo
		SetControllerType(ControllerType_DualAnalog);
	}
	
	void GameSettings::SetControllerType(ControllerType type)
	{
		m_ControllerType = type;
		
		ApplyFiltered(GameSetting_ControllerType);
	}
	
	void GameSettings::SetControllerModeMove(ControllerMode mode)
	{
		m_ControllerModeMove = mode;
		
		ApplyFiltered(GameSetting_ControllerModeMove);
	}
	
	void GameSettings::SetControllerModeFire(ControllerMode mode)
	{
		m_ControllerModeFire = mode;
		
		ApplyFiltered(GameSetting_ControllerModeFire);
	}
	
	void GameSettings::SetHudMode(HudMode mode)
	{
		m_HudMode = mode;
		
		ApplyFiltered(GameSetting_HudMode);
	}
	
	void GameSettings::SetHudOpacity(float opacity)
	{
		m_HudOpacity = opacity;
		
		ApplyFiltered(GameSetting_HudOpacity);
	}
	
	void GameSettings::SetCameraSemi3dEnabled(bool enabled)
	{
		m_CameraSemi3dEnabled = enabled;
		
		ApplyFiltered(GameSetting_CameraSemi3dEnabled);
	}

	//

	void GameSettings::ToggleBossEnabled()
	{
		m_CustomSettings.Boss_Toggle = !m_CustomSettings.Boss_Toggle;
	}
	void GameSettings::ToggleCustomBoss()
	{
		m_CustomSettings.Custom_Boss = !m_CustomSettings.Custom_Boss;
	}
	void GameSettings::ToggleWavesEnabled()
	{
		m_CustomSettings.Wave_Toggle = !m_CustomSettings.Wave_Toggle;
	}
	void GameSettings::ToggleMinesEnabled()
	{
		m_CustomSettings.Mines_Toggle = !m_CustomSettings.Mines_Toggle;
	}
	void GameSettings::ToggleClutterEnabled()
	{
		m_CustomSettings.Clutter_Toggle = !m_CustomSettings.Clutter_Toggle;
	}
	void GameSettings::TogglePowerUpEnabled()
	{
		m_CustomSettings.PowerUp_Toggle = !m_CustomSettings.PowerUp_Toggle;
	}
	void GameSettings::ToggleUpgradesUnlocked()
	{
		m_CustomSettings.UpgradesUnlock_Toggle = !m_CustomSettings.UpgradesUnlock_Toggle;
	}
	void GameSettings::ToggleInvuln()
	{
		Game::g_World->m_Player->Cheat_Invincibility();
		m_CustomSettings.Invuln_Toggle = !m_CustomSettings.Invuln_Toggle;
	}
	void GameSettings::ToggleInfiniteLives()
	{
		m_CustomSettings.InfLives_Toggle = !m_CustomSettings.InfLives_Toggle;
	}

	void GameSettings::AdjustLvl(int val)
	{
		m_CustomSettings.StartLevel += val;
		if(m_CustomSettings.StartLevel < 0 || m_CustomSettings.StartLevel  > 99)
			m_CustomSettings.StartLevel -= val;
	}

	void GameSettings::ResetCustomSettings()
	{
		m_CustomSettings.Boss_Toggle = true;
		m_CustomSettings.Custom_Boss = false;
		m_CustomSettings.Wave_Toggle = true;
		m_CustomSettings.Mines_Toggle = true;
		m_CustomSettings.Clutter_Toggle = true;
		m_CustomSettings.PowerUp_Toggle = true;
		m_CustomSettings.UpgradesUnlock_Toggle = false;
		m_CustomSettings.Invuln_Toggle = false;
		m_CustomSettings.InfLives_Toggle = false;
		m_CustomSettings.StartLevel = 0;
	}

	//
	
	const char* DifficultyToString()
	{
		switch (g_GameState->m_GameRound->Modifier_Difficulty_get())
		{
			case Difficulty_Easy:
				return "EASY";
			case Difficulty_Hard:
				return "HARD";
			case Difficulty_Custom:
				return "CUSTOM";
			
#ifndef DEPLOYMENT
			default:
				throw ExceptionNA();
#else
			default:
				return "???";
#endif
		}
	}
}
