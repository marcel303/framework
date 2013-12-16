#include "AnimTimer.h"
#include "Benchmark.h"
#include "Debugging.h"
#include "GameState.h"
#include "MenuMgr.h"
#include "Menu_BanditAnnounce.h"
#include "Menu_Cheats.h"
#include "Menu_Credits.h"
#include "Menu_CustomSettings.h"
#include "Menu_GameSelect.h"
#include "Menu_InGame.h"
#include "Menu_Main.h"
#include "Menu_Options.h"
#include "Menu_OptionsAdvanced.h"
#include "Menu_Paused.h"
#include "Menu_ScoreEntry.h"
#include "Menu_Scores.h"
#if defined(PSP_UI)
#include "Menu_ScoresPSP.h"
#endif
#include "Menu_ScoreSubmit.h"
#include "Menu_Upgrade.h"
#include "Menu_UpgradeBought.h"
#if defined(WIN32) || defined(LINUX) || defined(MACOS)
#include "Menu_GamepadSetup.h"
#endif
#include "TouchDLG.h"
#include "UsgResources.h"

#if USE_MENU_SELECT
namespace GameMenu
{
	class MenuCursor
	{
	public:
		MenuCursor()
		{
			mIsVisible = false;

			mAnimTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		}

		void HandleMenu(MenuType type)
		{
			mIsVisible = false;

			switch (type)
			{
			case MenuType_Cheats:
			case MenuType_Credits:
			case MenuType_CustomSettings:
			case MenuType_GameSelect:
			case MenuType_Main:
			case MenuType_Options:
			case MenuType_OptionsAdvanced:
			case MenuType_Pause:
			case MenuType_ScoreEntry:
			case MenuType_Scores:
#if defined(PSP_UI)
			case MenuType_ScoresPSP:
#endif
#if defined(WIN32) || defined(LINUX) || defined(MACOS)
			case MenuType_WinSetup:
#endif
			case MenuType_ScoreSubmit:
			case MenuType_Upgrade:
			case MenuType_UpgradeBought:
				mIsVisible = true;
				break;
			default:
				break;
			}
		}

		void StartAnim()
		{
			mAnimTimer.Start(AnimTimerMode_TimeBased, false, 0.7f, AnimTimerRepeat_None);
			mAnimTrigger.Start(4.0f);
		}

		void HandleFocus(GameMenu::IGuiElement* oldElement, GameMenu::IGuiElement* newElement)
		{
			if (newElement == 0)
				return;

			mTargetPosition = newElement->HitBoxCenter_get();

			if (oldElement == 0)
				mPosition = mTargetPosition;

			StartAnim();
		}

		void Update(float dt)
		{
			Vec2F delta = mTargetPosition - mPosition;
			mPosition += delta * powf(dt, 0.2f);

			if (mAnimTrigger.Read())
			{
				StartAnim();
			}
		}

		void Render()
		{
			if (mIsVisible == false)
				return;

			const float t = mAnimTimer.Progress_get();
			const float p = (cosf(t * Calc::m2PI) - 1.0f) * 0.5f * 10.0f;

#ifdef PSP
			g_GameState->Render(g_GameState->GetShape(Resources::MENU_CURSOR_PSP), mPosition + Vec2F(p, 0.0f), 0.0f, SpriteColor_MakeF(1.0f, 1.0f, 1.0f, 1.0f));
#else
			g_GameState->Render(g_GameState->GetShape(Resources::MENU_CURSOR), mPosition + Vec2F(p, 0.0f), 0.0f, SpriteColor_MakeF(1.0f, 1.0f, 1.0f, 1.0f));
#endif
		}

	private:
		bool mIsVisible;
		Vec2F mTargetPosition;
		Vec2F mPosition;
		TriggerTimerG mAnimTrigger;
		AnimTimer mAnimTimer;
	};

	static MenuCursor* gMenuCursor = 0;
}
#endif

namespace GameMenu
{
	MenuMgr::MenuMgr()
		: m_CursorEnabled(false)
		, m_ActiveMenu(MenuType_Undefined)
	{
		memset(m_Menus, 0, sizeof(m_Menus));
	}
	
	MenuMgr::~MenuMgr()
	{
		Reset();

#if USE_MENU_SELECT
		delete gMenuCursor;
		gMenuCursor = 0;
#endif
	}
	
	void MenuMgr::Initialize()
	{
		// register for touches
		
		TouchListener listener;
		listener.Setup(this, HandleTouchBegin, HandleTouchEnd, HandleTouchMove);
		g_GameState->m_TouchDLG->Register(USG::TOUCH_PRIO_INTERFACE, listener);
		g_GameState->m_TouchDLG->Enable(USG::TOUCH_PRIO_INTERFACE);
		
		// disable active menu
		
		m_ActiveMenu = MenuType_Undefined;
		
		// setup menus
		
		for (int i = 0; i < MenuType__End; ++i)
			m_Menus[i] = 0;
		
		m_Menus[MenuType_BanditIntro] = new Menu_BanditAnnounce();
		m_Menus[MenuType_Cheats] = new Menu_Cheats();
		m_Menus[MenuType_Credits] = new Menu_Credits();
		m_Menus[MenuType_CustomSettings] = new Menu_CustomSettings();
		m_Menus[MenuType_Empty] = new Menu();
		m_Menus[MenuType_GameSelect] = new Menu_GameSelect();
		m_Menus[MenuType_InGame] = new Menu_InGame();
		m_Menus[MenuType_Main] = new Menu_Main();
		m_Menus[MenuType_Options] = new Menu_Options();
		m_Menus[MenuType_OptionsAdvanced] = new Menu_OptionsAdvanced();
		m_Menus[MenuType_Pause] = new Menu_Pause();
		m_Menus[MenuType_ScoreEntry] = new Menu_ScoreEntry();
		m_Menus[MenuType_Scores] = new Menu_Scores();
#if defined(PSP_UI)
		m_Menus[MenuType_ScoresPSP] = new Menu_ScoresPSP();
#endif
		m_Menus[MenuType_ScoreSubmit] = new Menu_ScoreSubmit();
		m_Menus[MenuType_Upgrade] = new Menu_Upgrade();
		m_Menus[MenuType_UpgradeBought] = new Menu_UpgradeBought();
#if defined(WIN32) || defined(LINUX) || defined(MACOS)
		m_Menus[MenuType_WinSetup] = new Menu_WinSetup();
#endif
		
		// initialize menus
		
		for (int i = 0; i < MenuType__End; ++i)
		{
			if (m_Menus[i] == 0)
				throw ExceptionVA("menu not instantiated: %d", i);
			
			char initStr[32];
			sprintf(initStr, "init %d", i);
			UsingBegin(Benchmark bm(initStr))
				m_Menus[i]->Init();
			UsingEnd()
		}
		
#ifdef DEBUG
//		m_Menus[MenuType_InGame]->Add(BoundingBox2(Vec2F(450, 290), Vec2F(480, 320)), Textures::MENU_COLOR_WHITE, CallBack(this, &UpgradeButton));
#endif

#if USE_MENU_SELECT
		gMenuCursor = new MenuCursor();
#endif
	}
	
	void MenuMgr::Reset()
	{
		// destroy all menus
		
		for (int i = 0; i < MenuType__End; ++i)
		{
			delete m_Menus[i];
			m_Menus[i] = 0;
		}
	}
	
	void MenuMgr::Update(float dt)
	{
		for (int i = 0; i < MenuType__End; i++)
		{
			m_Menus[i]->Update(dt);
		}

#if USE_MENU_SELECT
		if (m_CursorEnabled)
		{
			gMenuCursor->Update(dt);
		}
#endif
	}

	void MenuMgr::Render()
	{
		for (int i = 0; i < MenuType__End; i++)
		{
			m_Menus[i]->Render();
		}

#if USE_MENU_SELECT
		if (m_CursorEnabled)
		{
			gMenuCursor->Render();
		}
#endif
	}	

	bool MenuMgr::HandleEvent(const Event& e)
	{
		if (m_Menus[m_ActiveMenu]->HandleEvent(e) == true)
		{
			return true;
		}

		switch (e.type)
		{
			case EVT_KEY:
			{
				switch (e.key.key)
				{
					case IK_LEFT:
					case IK_RIGHT:
					case IK_UP:
					case IK_DOWN:
						if (m_CursorEnabled)
						{
							if (e.key.state)
							{
								int dx = e.key.key == IK_LEFT ? -1 : e.key.key == IK_RIGHT ? +1 : 0;
								int dy = e.key.key == IK_UP ? -1 : e.key.key == IK_DOWN ? +1 : 0;
								m_Menus[m_ActiveMenu]->MoveSelection(dx, dy);
							}
							return true;
						}
				}
				break;
			}

			default:
				break;
		}
		
		if (m_CursorEnabled)
		{
			if (e.type == EVT_MENU_LEFT)
			{
				m_Menus[m_ActiveMenu]->MoveSelection(-1, 0);
				return true;
			}
			if (e.type == EVT_MENU_RIGHT)
			{
				m_Menus[m_ActiveMenu]->MoveSelection(+1, 0);
				return true;
			}
			if (e.type == EVT_MENU_UP)
			{
				m_Menus[m_ActiveMenu]->MoveSelection(0, -1);
				return true;
			}
			if (e.type == EVT_MENU_DOWN)
			{
				m_Menus[m_ActiveMenu]->MoveSelection(0, +1);
				return true;
			}
		}

		if (e.type == EVT_PAUSE)
		{
			return m_Menus[m_ActiveMenu]->HandlePause();
		}

		if (e.type == EVT_MENU_BACK)
		{
			m_Menus[m_ActiveMenu]->HandleBack();
			return true;
		}

		if (m_CursorEnabled)
		{
			if (e.type == EVT_MENU_NEXT)
			{
				m_Menus[m_ActiveMenu]->MoveNext();
				return true;
			}

			if (e.type == EVT_MENU_PREV)
			{
				m_Menus[m_ActiveMenu]->MovePrev();
				return true;
			}
		}

		return false;
	}
	
	Menu* MenuMgr::ActiveMenu_get()
	{
		if (m_ActiveMenu == MenuType_Undefined)
			return 0;
		else
			return m_Menus[m_ActiveMenu];
	}

	void MenuMgr::ActiveMenu_set(MenuType menu)
	{
		if (m_ActiveMenu != MenuType_Undefined)
			m_Menus[m_ActiveMenu]->Deactivate();
		
		m_Menus[menu]->Activate();
		
		m_ActiveMenu = menu;

#if USE_MENU_SELECT
		gMenuCursor->HandleMenu(menu);
#endif
	}
	
	void MenuMgr::CursorEnabled_set(bool enabled)
	{
		m_CursorEnabled = enabled;

		if (enabled)
		{
			Menu * activeMenu = ActiveMenu_get();

			if (activeMenu)
			{
				activeMenu->MoveToDefault();
			}
		}
	}

	Menu* MenuMgr::Menu_get(MenuType menu)
	{ 
		return m_Menus[menu]; 
	}

	void MenuMgr::HandleFocus(IGuiElement* oldElement, IGuiElement* newElement)
	{
#if USE_MENU_SELECT
		gMenuCursor->HandleFocus(oldElement, newElement);
#endif
	}

	bool MenuMgr::HandleTouchBegin(void* obj, const TouchInfo& touchInfo)
	{
		MenuMgr* self = (MenuMgr*)obj;
		
		for (int i = 0; i < MenuType__End; i++)
		{
			if (self->m_Menus[i]->HandleTouchBegin(touchInfo))
				return true;
		}
		
		return false;
	}
	
	bool MenuMgr::HandleTouchMove(void* obj, const TouchInfo& touchInfo)
	{
		MenuMgr* self = (MenuMgr*)obj;
		
		for (int i = 0; i < MenuType__End; i++)
		{
			if (self->m_Menus[i]->HandleTouchMove(touchInfo))
				return true;
		}
		
		return false;
	}
	
	bool MenuMgr::HandleTouchEnd(void* obj, const TouchInfo& touchInfo)
	{
		MenuMgr* self = (MenuMgr*)obj;
		
		for (int i = 0; i < MenuType__End; i++)
		{
			if (self->m_Menus[i]->HandleTouchEnd(touchInfo))
				return true;
		}
		
		return false;
	}
}
