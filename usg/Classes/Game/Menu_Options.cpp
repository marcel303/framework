#include "GameSettings.h"
#include "GameState.h"
#include "GuiButton.h"
#include "GuiCheckbox.h"
#include "GuiSlider.h"
#include "Menu_Options.h"
#include "MenuMgr.h"
#include "MenuRender.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_Credits.h"
#include "View_Options.h"

// todo: copy gauge button from View_Options
// todo: set custom render volume buttons

#define VOLUME_PREVIEW_INTERVAL 0.07f

#define BUTTON_TILT_ENABLED "use tilt control"

namespace GameMenu
{
#if defined(PSP_UI)
	static DesignInfo design[] =
	{
		DesignInfo("sound_enabled",      true,  Vec2F(30.0f,  85.0f ), Vec2F(30.0f,  30.0f)),
		DesignInfo("sound_volume2",      true,  Vec2F(190.0f, 110.0f), Vec2F(240.0f, 30.0f)),
		DesignInfo("music_enabled",      true,  Vec2F(30.0f,  150.0f), Vec2F(30.0f,  30.0f)),
		DesignInfo("music_volume2",      true,  Vec2F(190.0f, 180.0f), Vec2F(240.0f, 30.0f)),
		DesignInfo("credits",            false, Vec2F(),               Vec2F()             ),
		DesignInfo(BUTTON_TILT_ENABLED,  false, Vec2F(),               Vec2F()             )
	};
#else
	static DesignInfo design[] =
	{
		  DesignInfo("sound_enabled",      true,  Vec2F(30.0f,  55.0f ), Vec2F(30.0f,  30.0f))
		, DesignInfo("sound_volume2",      true,  Vec2F(190.0f, 80.0f ), Vec2F(240.0f, 30.0f))
		, DesignInfo("music_enabled",      true,  Vec2F(30.0f,  120.0f), Vec2F(30.0f,  30.0f))
		, DesignInfo("music_volume2",      true,  Vec2F(190.0f, 150.0f), Vec2F(240.0f, 30.0f))
#if defined(BBOS) && 0
		, DesignInfo(BUTTON_TILT_ENABLED,  true, Vec2F(30.0f,  195.0f), Vec2F(30.0f,  30.0f))
#else
		, DesignInfo(BUTTON_TILT_ENABLED,  false, Vec2F(30.0f,  195.0f), Vec2F(30.0f,  30.0f))
#endif
#if defined(IPHONEOS) && !defined(IPAD)
		, DesignInfo("flip_screen",        true,  Vec2F(30.0f,  195.0f), Vec2F(30.0f,  30.0f))
#else
		, DesignInfo("flip_screen",        false,  Vec2F(30.0f,  195.0f), Vec2F(30.0f,  30.0f))
#endif
	};
#endif

	enum OptionType
	{
		OptionType_EnhancedGraphicEnabled,
		OptionType_MusicEnabled,
		OptionType_MusicVolume,
		OptionType_SoundEnabled,
		OptionType_SoundVolume
	};
	
	Menu_Options::Menu_Options() : Menu()
	{
	}
	
	Menu_Options::~Menu_Options()
	{
	}
	
	void Menu_Options::Init()
	{
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.3f);

		AddElement(new GuiCheckbox("sound_enabled", true, "sound enabled", -1, CallBack(this, Handle_ToggleSound)));
		AddElement(new GuiSlider("sound_volume2", 0.0f, 1.0f, 0.0f, "volume", -1, OptionType_SoundVolume, CallBack(this, Handle_VolumeChange)));
		AddElement(new GuiCheckbox("music_enabled", true, "music enabled", -1, CallBack(this, Handle_ToggleMusic)));
		AddElement(new GuiSlider("music_volume2", 0.0f, 1.0f, 0.0f, "volume", -1, OptionType_MusicVolume, CallBack(this, Handle_VolumeChange)));
		AddElement(new GuiCheckbox(BUTTON_TILT_ENABLED, false, BUTTON_TILT_ENABLED, -1, CallBack(this, Handle_ToggleAlternateControls)));
		AddElement(new GuiCheckbox("flip_screen", true, "flip screen", -1, CallBack(this, Handle_ToggleScreenFlip)));

		AddButton(Button::Make_Shape("credits", Vec2F(360.0f, 205.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_CREDITS ), 0, CallBack(this, Handle_Credits )));
		AddButton(Button::Make_Shape(0,         Vec2F(360.0f, 240.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_ADVANCED), 0, CallBack(this, Handle_Advanced)));
		AddButton(Button::Make_Shape(0,         Vec2F(10.0f,  240.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_ACCEPT  ), 0, CallBack(this, Handle_Accept  )));
		AddButton(Button::Make_Shape(0,         Vec2F(110.0f, 240.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_DISMISS ), 0, CallBack(this, Handle_Dismiss )));
		
		m_SoundVolumePreviewTimer.Start(VOLUME_PREVIEW_INTERVAL);

		DESIGN_APPLY(this, design);

		Translate((VIEW_SX-480)/2, (VIEW_SY-320)/2);
	}
	
	void Menu_Options::HandleFocus()
	{
		Menu::HandleFocus();

		FindCheckbox("sound_enabled")->IsChecked_set(g_GameState->m_GameSettings->m_SoundEnabled, false);
		FindSlider(  "sound_volume2")->Value_set(    g_GameState->m_GameSettings->m_SoundVolume,  false);
		FindCheckbox("music_enabled")->IsChecked_set(g_GameState->m_GameSettings->m_MusicEnabled, false);
		FindSlider(  "music_volume2")->Value_set(    g_GameState->m_GameSettings->m_MusicVolume,  false);

#if defined(BBOS)
		FindCheckbox(BUTTON_TILT_ENABLED)->IsChecked_set(g_GameState->m_GameSettings->m_ControllerType == ControllerType_Tilt, false);
#else
		FindCheckbox(BUTTON_TILT_ENABLED)->IsChecked_set(g_GameState->m_GameSettings->m_ControllerType == ControllerType_DualAnalog, false);
#endif
		FindCheckbox("flip_screen")->IsChecked_set(g_GameState->m_GameSettings->m_ScreenFlip, false);
	}

	void Menu_Options::HandleBack()
	{
#if defined(PSP_UI)
		Handle_Accept(this, 0);
#else
		Handle_Dismiss(this, 0);
#endif
	}

	//
	
	void Menu_Options::Handle_ToggleSound(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->ToggleSoundEnabled();
		
		Assert(element->IsChecked_get() == g_GameState->m_GameSettings->m_SoundEnabled);
		element->IsChecked_set(g_GameState->m_GameSettings->m_SoundEnabled, false);
	}
	
	void Menu_Options::Handle_ToggleMusic(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->ToggleMusicEnabled();
		
		Assert(element->IsChecked_get() == g_GameState->m_GameSettings->m_MusicEnabled);
		element->IsChecked_set(g_GameState->m_GameSettings->m_MusicEnabled, false);
	}
	
	void Menu_Options::Handle_ToggleAlternateControls(void* obj, void* arg)
	{
		Menu_Options* self = (Menu_Options*)obj;
		
#if defined(BBOS)
		if (g_GameState->m_GameSettings->m_ControllerType == ControllerType_Tilt)
			g_GameState->m_GameSettings->SetControllerType(ControllerType_DualAnalog);
		else
			g_GameState->m_GameSettings->SetControllerType(ControllerType_Tilt);

		self->FindCheckbox(BUTTON_TILT_ENABLED)->IsChecked_set(g_GameState->m_GameSettings->m_ControllerType == ControllerType_Tilt, false);
#else
		g_GameState->m_GameSettings->SwitchControllerType();

		//Assert(element->IsChecked_get() == (g_GameState->m_GameSettings.m_ControllerType == ControllerType_DualAnalog));
		self->FindCheckbox(BUTTON_TILT_ENABLED)->IsChecked_set(g_GameState->m_GameSettings->m_ControllerType == ControllerType_DualAnalog, false);
#endif
	}
	
	void Menu_Options::Handle_ToggleScreenFlip(void* obj, void* arg)
	{
		GuiCheckbox* element = (GuiCheckbox*)arg;
		
		g_GameState->m_GameSettings->m_ScreenFlip = !g_GameState->m_GameSettings->m_ScreenFlip;
		
		Assert(element->IsChecked_get() == g_GameState->m_GameSettings->m_ScreenFlip);
		element->IsChecked_set(g_GameState->m_GameSettings->m_ScreenFlip, false);
	}
	
	void Menu_Options::Handle_VolumeChange(void* obj, void* arg)
	{
		Menu_Options* self = (Menu_Options*)obj;
		
		GuiSlider* element = (GuiSlider*)arg;
		
		OptionType type = (OptionType)element->mTag;
		
		const float volume = element->Value_get();
		
		switch (type)
		{
			case OptionType_SoundVolume:
				g_GameState->m_GameSettings->SetSoundVolume(volume);
				
				if (self->m_SoundVolumePreviewTimer.Read())
				{
					g_GameState->m_SoundEffects->Play(Resources::OPTIONSVIEW_VOLUME_SET, 0);
					
					self->m_SoundVolumePreviewTimer.Start(VOLUME_PREVIEW_INTERVAL);
				}
				break;
			case OptionType_MusicVolume:
				g_GameState->m_GameSettings->SetMusicVolume(volume);
				break;
			default:
				throw ExceptionVA("not implemented");
		}
	}
	
	void Menu_Options::Handle_Advanced(void* obj, void* arg)
	{
		g_GameState->Interface_get()->ActiveMenu_set(MenuType_OptionsAdvanced);
	}
	
	void Menu_Options::Handle_Credits(void* obj, void* arg)
	{
		Game::View_Credits* view = (Game::View_Credits*)g_GameState->GetView(::View_Credits);
		
		view->Show(::View_Options);
	}
	
	void Menu_Options::Handle_Accept(void* obj, void* arg)
	{
		Game::View_Options* view = (Game::View_Options*)g_GameState->GetView(::View_Options);
		
		g_GameState->m_GameSettings->Save();
		
		g_GameState->ActiveView_set(view->m_NextView);
	}
	
	void Menu_Options::Handle_Dismiss(void* obj, void* arg)
	{
		Game::View_Options* view = (Game::View_Options*)g_GameState->GetView(::View_Options);
		
		g_GameState->m_GameSettings->Load();

		g_GameState->m_GameSettings->Apply();
		
		g_GameState->ActiveView_set(view->m_NextView);
	}
}
