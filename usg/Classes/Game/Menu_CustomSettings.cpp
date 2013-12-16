#include "GameSettings.h"
#include "GameState.h"
#include "GuiButton.h"
#include "GuiCheckbox.h"
#include "Menu_CustomSettings.h"
#include "MenuRender.h"
#include "StringBuilder.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_GameSelect.h"
#include "World.h"

namespace GameMenu
{
	static void Handle_Nothing(void* obj, void* arg)
	{
	}

	static DesignInfo design[] =
	{
		DesignInfo("boss",         true, Vec2F(30.0f,  55.0f ), Vec2F(180.0f, 30.0f)),
		DesignInfo("enemies",      true, Vec2F(30.0f,  105.0f), Vec2F(180.0f, 30.0f)),
		DesignInfo("mines",        true, Vec2F(30.0f,  155.0f), Vec2F(180.0f, 30.0f)),
		DesignInfo("clutter",      true, Vec2F(30.0f,  205.0f), Vec2F(180.0f, 30.0f)), 
		DesignInfo("mini bosses",  true, Vec2F(230.0f, 55.0f ), Vec2F(180.0f, 30.0f)),
		DesignInfo("upgrades",     true, Vec2F(230.0f, 105.0f), Vec2F(180.0f, 30.0f)),
		DesignInfo("invulnerable", true, Vec2F(230.0f, 155.0f), Vec2F(180.0f, 30.0f)),
		DesignInfo("lives",        true, Vec2F(230.0f, 205.0f), Vec2F(180.0f, 30.0f))
	};

	Menu_CustomSettings::Menu_CustomSettings() : Menu()
	{
	}
	
	Menu_CustomSettings::~Menu_CustomSettings()
	{
	}

	static void Render_Level(void* obj, void* arg)
	{
		Button* button = (Button*)arg;
		
		StringBuilder<32> sb;
		sb.Append(g_GameState->m_GameSettings->m_CustomSettings.StartLevel);
		RenderText(button->Position_get(), button->Size_get(), g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColors::White, TextAlignment_Center, TextAlignment_Top, true, sb.ToString());
	}

	void Menu_CustomSettings::Init()
	{
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.5f);

		AddButton(Button::Make_Text(0, Vec2F(10.0f, 30.0f), Vec2F(0.0f, 0.0f), "please select your modifications", 0, CallBack(this, Handle_Nothing)));

		AddElement(new GuiCheckbox("boss", true, "boss", -1, CallBack(this, Handle_Boss_Toggle)));
		AddButton(Button::Make_TextEx(0, Vec2F(30.0f, 85.0f), Resources::FONT_LGS, "toggle boss fights", 0, CallBack(this, Handle_Nothing)));

		//AddButton(Button::Make_Custom("alt boss", Vec2F(230.0f, 255.0f), Vec2F(30.0f, 30.0f), 0,

		AddElement(new GuiCheckbox("enemies", true, "enemies", -1, CallBack(this, Handle_Waves_Toggle)));
		AddButton(Button::Make_TextEx(0, Vec2F(30.0f, 135.0f), Resources::FONT_LGS, "toggle enemies", 0, CallBack(this, Handle_Nothing)));

		AddElement(new GuiCheckbox("mines", true, "mines", -1, CallBack(this, Handle_Mines_Toggle)));
		AddButton(Button::Make_TextEx(0, Vec2F(30.0f, 185.0f), Resources::FONT_LGS, "toggle mines", 0, CallBack(this, Handle_Nothing)));

		AddElement(new GuiCheckbox("clutter", true, "clutter", -1, CallBack(this, Handle_Clutter_Toggle)));
		AddButton(Button::Make_TextEx(0, Vec2F(30.0f, 235.0f), Resources::FONT_LGS, "toggle clutter", 0, CallBack(this, Handle_Nothing)));

		//AddButton(Button::Make_Text(0, Vec2F(245.0f, 265.0f), Vec2F(30.0f, 30.0f),"0", 0, CallBack(this, Handle_Nothing)));

		AddElement(new GuiCheckbox("mini bosses", true, "mini bosses", -1, CallBack(this, Handle_PowerUp_Toggle)));
		AddButton(Button::Make_TextEx(0, Vec2F(230.0f, 85.0f), Resources::FONT_LGS, "toggle mini bosses & powerups", 0, CallBack(this, Handle_Nothing)));

		AddElement(new GuiCheckbox("upgrades", true, "upgrades", -1, CallBack(this, Handle_UpgradesUnlock_Toggle)));
		AddButton(Button::Make_TextEx(0, Vec2F(230.0f, 135.0f), Resources::FONT_LGS, "unlock all upgrades", 0, CallBack(this, Handle_Nothing)));

		AddElement(new GuiCheckbox("invulnerable", true, "invulnerable", -1, CallBack(this, Handle_Invuln_Toggle)));
		AddButton(Button::Make_TextEx(0, Vec2F(230.0f, 185.0f), Resources::FONT_LGS, "toggle invulnerability", 0, CallBack(this, Handle_Nothing)));

		AddElement(new GuiCheckbox("lives", true, "lives", -1, CallBack(this, Handle_InfLives_Toggle)));
		AddButton(Button::Make_TextEx(0, Vec2F(230.0f, 235.0f), Resources::FONT_LGS, "toggle infinite lives", 0, CallBack(this, Handle_Nothing)));

		AddButton(Button::Make_TextEx2("lower", Vec2F(110.0f, 255.0f), Vec2F(100.0f, 0.0f), Resources::FONT_USUZI_SMALL, TextAlignment_Right, "LOWER", 0, CallBack(this, Handle_LevelDown)));
		AddButton(Button::Make_TextEx2("raise", Vec2F(250.0f, 255.0f), Vec2F(100.0f, 0.0f), Resources::FONT_USUZI_SMALL, TextAlignment_Left, "RAISE", 0, CallBack(this, Handle_LevelUp)));
		//AddButton(Button::Make_Custom("lower", Vec2F(130.0f, 245.0f), Vec2F(100.0f, 30.0f), 0, CallBack(this, Handle_LevelDown), CallBack(this, Render_LevelLower)));
		//AddButton(Button::Make_Custom("raise", Vec2F(230.0f, 245.0f), Vec2F(130.0f, 30.0f), 0, CallBack(this, Handle_LevelUp), CallBack(this, Render_LevelBox)));
		AddButton(Button::Make_Custom("level", Vec2F(220.0f, 255.0f), Vec2F(20.0f, 30.0f), 0, CallBack(this, Handle_LevelUp), CallBack(this, Render_Level)));
		FindButton("level")->m_IsEnabled = false;
		AddButton(Button::Make_TextEx(0, Vec2F(130.0f, 270.0f), Resources::FONT_LGS, "adjust the starting level", 0, CallBack(this, Handle_Nothing)));

		AddButton(Button::Make_TextEx2(0, Vec2F(210.0f, 280.0f), Vec2F(40.0f, 0.0f), Resources::FONT_USUZI_SMALL, TextAlignment_Center, "GO!", 0, CallBack(this, Handle_CustomStart)));

		for (Col::ListNode<IGuiElement*>* node = m_Elements.m_Head; node; node = node->m_Next)
		{
			if (node->m_Object->Type_get() != GuiElementType_Button)
				continue;

			Button* button = (Button*)node->m_Object;

			if (button->OnClick.CallBack_get() == Handle_Nothing)
				button->m_IsEnabled = false;
		}

		DESIGN_APPLY(this, design);

		Translate((VIEW_SX-480)/2, (VIEW_SY-320)/2);
	}

	void Menu_CustomSettings::HandleFocus()
	{
		Menu::HandleFocus();

		FindCheckbox("boss"        )->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Boss_Toggle,           false);
		//FindCheckbox("alt boss"    )->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Custom_Boss,           false);
		FindCheckbox("enemies"     )->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Wave_Toggle,           false);
		FindCheckbox("mines"       )->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Mines_Toggle,          false);
		FindCheckbox("clutter"     )->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Clutter_Toggle,        false);
		FindCheckbox("mini bosses" )->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.PowerUp_Toggle,        false);
		FindCheckbox("upgrades"    )->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.UpgradesUnlock_Toggle, false);
		FindCheckbox("invulnerable")->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Invuln_Toggle,         false);
		FindCheckbox("lives"       )->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.InfLives_Toggle,       false);

		FindButton("raise")->m_Info = 0;
		FindButton("lower")->m_Info = 0;
	}

	void Menu_CustomSettings::HandleBack()
	{
		g_GameState->ActiveView_set(::View_GameSelect);
	}

	void Menu_CustomSettings::Handle_CustomStart(void* obj, void* arg)
	{
		g_GameState->GameBegin(Game::GameMode_ClassicPlay, Difficulty_Custom, false);
	}

	void Menu_CustomSettings::Handle_Boss_Toggle(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->ToggleBossEnabled();
		
		element->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Boss_Toggle, false);
	}

	void Menu_CustomSettings::Handle_Custom_Boss(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->ToggleCustomBoss();
		
		element->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Custom_Boss, false);
	}

	void Menu_CustomSettings::Handle_Waves_Toggle(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->ToggleWavesEnabled();
		
		element->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Wave_Toggle, false);
	}

	void Menu_CustomSettings::Handle_Mines_Toggle(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->ToggleMinesEnabled();
		
		element->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Mines_Toggle, false);
	}

	void Menu_CustomSettings::Handle_Clutter_Toggle(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->ToggleClutterEnabled();
		
		element->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Clutter_Toggle, false);
	}

	void Menu_CustomSettings::Handle_PowerUp_Toggle(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->TogglePowerUpEnabled();
		
		element->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.PowerUp_Toggle, false);
	}

	void Menu_CustomSettings::Handle_UpgradesUnlock_Toggle(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->ToggleUpgradesUnlocked();
		
		element->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.UpgradesUnlock_Toggle, false);
	}

	void Menu_CustomSettings::Handle_Invuln_Toggle(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->ToggleInvuln();
		
		element->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.Invuln_Toggle, false);
	}

	void Menu_CustomSettings::Handle_InfLives_Toggle(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->ToggleInfiniteLives();
		
		element->IsChecked_set(g_GameState->m_GameSettings->m_CustomSettings.InfLives_Toggle, false);
	}

	void Menu_CustomSettings::Handle_LevelUp(void* obj, void* arg)
	{
		g_GameState->m_GameSettings->AdjustLvl(1);
	}

	void Menu_CustomSettings::Handle_LevelDown(void* obj, void* arg)
	{
		g_GameState->m_GameSettings->AdjustLvl(-1);
	}
}
