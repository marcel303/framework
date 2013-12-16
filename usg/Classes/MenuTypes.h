#pragma once

namespace GameMenu
{
	enum MenuType
	{
		MenuType_Undefined = -1,
		MenuType_BanditIntro,
		MenuType_Cheats,
		MenuType_Credits,
		MenuType_CustomSettings,
		MenuType_Empty,
		MenuType_InGame,
		MenuType_GameSelect,
		MenuType_Main,
		MenuType_Options,
		MenuType_OptionsAdvanced,
		MenuType_Pause,
		MenuType_ScoreEntry,
		MenuType_Scores,
#if defined(PSP_UI)
		MenuType_ScoresPSP,
#endif
		MenuType_ScoreSubmit,
		MenuType_Upgrade,
		MenuType_UpgradeBought,
#if defined(WIN32) || defined(LINUX) || defined(MACOS)
		MenuType_WinSetup,
#endif
		MenuType__End
	};
	
	enum TransitionEffect
	{
		TransitionEffect_None,
		TransitionEffect_Menu, // use transition effect configured globally for menu
		TransitionEffect_Fade,
		TransitionEffect_Slide
	};
	
	enum HitEffect
	{
		HitEffect_None,
		HitEffect_Particles
	};
	
	enum ViewNameSnap
	{
		ViewNameSnap_TopRight,
		ViewNameSnap_BottomRight
	};
	
	extern void ViewName_Render(void* obj, void* arg);
	
	//
	
	enum MenuState
	{
		MenuState_Undefined,
		MenuState_DEACTIVE,
		MenuState_ACTIVATE,
		MenuState_ACTIVE,
		MenuState_DEACTIVATE
	};
}
