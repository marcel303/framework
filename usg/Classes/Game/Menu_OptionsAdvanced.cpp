#include "GameSettings.h"
#include "GameState.h"
#include "GuiButton.h"
#include "GuiCheckbox.h"
#include "GuiSlider.h"
#include "Menu_Options.h"
#include "Menu_OptionsAdvanced.h"
#include "MenuMgr.h"
#include "MenuRender.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_Credits.h"

#define BUTTON_CALIBRATE_MOVE "auto-calibrate move button"
#define BUTTON_CALIBRATE_FIRE "auto-calibrate fire button"
#define BUTTON_CAMERA_3D "3d effect"
#define BUTTON_HUD_OPACITY "hud_opacity"
#define BUTTON_TILT_ENABLED "use tilt control"

namespace GameMenu
{
#ifdef PSP_UI
	static DesignInfo design[] =
	{
		  DesignInfo(BUTTON_CALIBRATE_MOVE, false, Vec2F(0.0f,   0.0f ), Vec2F(0.0f,   0.0f))
		, DesignInfo(BUTTON_CALIBRATE_FIRE, false, Vec2F(0.0f,   0.0f ), Vec2F(0.0f,   0.0f))
		, DesignInfo(BUTTON_HUD_OPACITY,    true,  Vec2F(210.0f, 85.0f), Vec2F(200.0f, 30.0f))
		, DesignInfo(BUTTON_CAMERA_3D,      false, Vec2F(0.0f,   0.0f ), Vec2F(0.0f,   0.0f))
	};
#else
	static DesignInfo design[] =
	{
		  DesignInfo(BUTTON_CALIBRATE_MOVE, true, Vec2F(30.0f,  55.0f ), Vec2F(30.0f,  30.0f))
		, DesignInfo(BUTTON_CALIBRATE_FIRE, true, Vec2F(30.0f,  90.0f ), Vec2F(30.0f,  30.0f))
		, DesignInfo(BUTTON_HUD_OPACITY,    true, Vec2F(210.0f, 121.0f), Vec2F(200.0f, 30.0f))
		, DesignInfo(BUTTON_CAMERA_3D,      true, Vec2F(30.0f,  160.0f), Vec2F(30.0f,  30.0f))
#if defined(IPHONEOS)
		, DesignInfo(BUTTON_TILT_ENABLED,   true, Vec2F(30.0f,  195.0f), Vec2F(30.0f,  30.0f))
#endif
	};
#endif

	void Menu_OptionsAdvanced::Init()
	{
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.3f);
			
		// controls: move mode
		AddElement(new GuiCheckbox(BUTTON_CALIBRATE_MOVE, true, BUTTON_CALIBRATE_MOVE, -1, CallBack(this, Handle_ControllerMode_Move)));
		// controls: fire mode
		AddElement(new GuiCheckbox(BUTTON_CALIBRATE_FIRE, true, BUTTON_CALIBRATE_FIRE, -1, CallBack(this, Handle_ControllerMode_Fire)));

		// hud: opacity
		AddElement(new GuiSlider(BUTTON_HUD_OPACITY, 0.0f, 1.0f, 0.0f, "hud opacity", -1, 0, CallBack(this, Handle_HudOpacity)));
		
		// camera: 3d effect
		AddElement(new GuiCheckbox(BUTTON_CAMERA_3D, true, BUTTON_CAMERA_3D, -1, CallBack(this, Handle_Camera3dEnabled)));
		
#if defined(IPHONEOS)
		// controls: tilt enabled
		AddElement(new GuiCheckbox(BUTTON_TILT_ENABLED, true, BUTTON_TILT_ENABLED, -1, CallBack(this, Handle_ControllerType_Toggle)));
#endif
		
#ifdef PSP_UI
		AddButton(Button::Make_Shape("credits", Vec2F(360.0f, 205.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_CREDITS ), 0, CallBack(this, Handle_Credits )));
#endif
		AddButton(Button::Make_Shape(0, Vec2F(360.0f, 240.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_BASIC), 0, CallBack(this, Handle_Basic)));
		AddButton(Button::Make_Shape(0, Vec2F(10.0f, 240.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_ACCEPT), 0, CallBack(this, Handle_Accept)));
		AddButton(Button::Make_Shape(0, Vec2F(110.0f, 240.0f), g_GameState->GetShape(Resources::OPTIONSVIEW_DISMISS), 0, CallBack(this, Handle_Dismiss)));

		DESIGN_APPLY(this, design);

		Translate((VIEW_SX-480)/2, (VIEW_SY-320)/2);
	}

	void Menu_OptionsAdvanced::HandleFocus()
	{
		Menu::HandleFocus();

		FindCheckbox(BUTTON_CALIBRATE_MOVE)->IsChecked_set(g_GameState->m_GameSettings->m_ControllerModeMove == Game::ControllerMode_AutoCalibrate, false);
		FindCheckbox(BUTTON_CALIBRATE_FIRE)->IsChecked_set(g_GameState->m_GameSettings->m_ControllerModeFire == Game::ControllerMode_AutoCalibrate, false);
		FindCheckbox(BUTTON_CAMERA_3D     )->IsChecked_set(g_GameState->m_GameSettings->m_CameraSemi3dEnabled, false);
		FindSlider  (BUTTON_HUD_OPACITY   )->Value_set(g_GameState->m_GameSettings->m_HudOpacity, false);
		
#if defined(IPHONEOS)
		FindCheckbox(BUTTON_TILT_ENABLED)->IsChecked_set(g_GameState->m_GameSettings->m_ControllerType == ControllerType_Tilt, false);
#endif
	}

	void Menu_OptionsAdvanced::HandleBack()
	{
		Handle_Dismiss(this, 0);
	}

	bool Menu_OptionsAdvanced::Render()
	{
		if (!Menu::Render())
			return false;
		
		//int ox = (VIEW_SX-480)/2;
		//int oy = (VIEW_SY-320)/2;

//		RenderText(Vec2F((float)ox, oy+210.0f), Vec2F(480.0f, 10.0f), g_GameState->GetFont(Resources::FONT_CALIBRI_SMALL), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true,
//				   "when calibration is enabled, the initial touch location will be used as resting point");
		
		//

#if 0
		Particle& p = g_GameState->m_ParticleEffect_UI.Allocate(g_GameState->GetTexture(Textures::EXPLOSION_LINE), 0, Particle_Default_Update);
		
		for (int i = 0; i < 4; ++i)
			Particle_Default_Setup(&p, Calc::Random(VIEW_SX), Calc::Random(VIEW_SY), 0.5f, Calc::Random(10.0f, 30.0f), 2.0f, 0.0f, 400.0f);
		
		p.m_Color = SpriteColors::Red;
#endif

		return true;
	}
	
	void Menu_OptionsAdvanced::Handle_ControllerMode_Move(void* obj, void* arg)
	{
		Menu_OptionsAdvanced* self = (Menu_OptionsAdvanced*)obj;
		
		g_GameState->m_GameSettings->SetControllerModeMove((Game::ControllerMode)((g_GameState->m_GameSettings->m_ControllerModeMove + 1) % Game::ControllerMode__Count));
		
		self->FindCheckbox(BUTTON_CALIBRATE_MOVE)->IsChecked_set(g_GameState->m_GameSettings->m_ControllerModeMove == Game::ControllerMode_AutoCalibrate, false);
	}

	void Menu_OptionsAdvanced::Handle_ControllerMode_Fire(void* obj, void* arg)
	{
		Menu_OptionsAdvanced* self = (Menu_OptionsAdvanced*)obj;
		
		g_GameState->m_GameSettings->SetControllerModeFire((Game::ControllerMode)((g_GameState->m_GameSettings->m_ControllerModeFire + 1) % Game::ControllerMode__Count));
		
		self->FindCheckbox(BUTTON_CALIBRATE_FIRE)->IsChecked_set(g_GameState->m_GameSettings->m_ControllerModeFire == Game::ControllerMode_AutoCalibrate, false);
	}
	
	void Menu_OptionsAdvanced::Handle_HudOpacity(void* obj, void* arg)
	{
		GuiSlider* element = (GuiSlider*)arg;
		
		g_GameState->m_GameSettings->SetHudOpacity(element->Value_get());
		
		element->Value_set(g_GameState->m_GameSettings->m_HudOpacity, false);
	}
	
	void Menu_OptionsAdvanced::Handle_Camera3dEnabled(void* obj, void* arg)
	{
		Menu_OptionsAdvanced* self = (Menu_OptionsAdvanced*)obj;

		g_GameState->m_GameSettings->SetCameraSemi3dEnabled(!g_GameState->m_GameSettings->m_CameraSemi3dEnabled);

		self->FindCheckbox(BUTTON_CAMERA_3D)->IsChecked_set(g_GameState->m_GameSettings->m_CameraSemi3dEnabled, false);
	}
	
	void Menu_OptionsAdvanced::Handle_ControllerType_Toggle(void* obj, void* arg)
	{
		Menu_OptionsAdvanced* self = (Menu_OptionsAdvanced*)obj;
		
		if (g_GameState->m_GameSettings->m_ControllerType == ControllerType_Tilt)
			g_GameState->m_GameSettings->SetControllerType(ControllerType_DualAnalog);
		else
			g_GameState->m_GameSettings->SetControllerType(ControllerType_Tilt);
		
		self->FindCheckbox(BUTTON_TILT_ENABLED)->IsChecked_set(g_GameState->m_GameSettings->m_ControllerType == ControllerType_Tilt, false);
	}
	
	void Menu_OptionsAdvanced::Handle_Basic(void* obj, void* arg)
	{
		g_GameState->Interface_get()->ActiveMenu_set(MenuType_Options);
	}

	void Menu_OptionsAdvanced::Handle_Credits(void* obj, void* arg)
	{
		Game::View_Credits* view = (Game::View_Credits*)g_GameState->GetView(::View_Credits);
		
		view->Show(::View_Options);
	}
	
	void Menu_OptionsAdvanced::Handle_Accept(void* obj, void* arg)
	{
		Menu_Options::Handle_Accept(obj, arg);
	}
	
	void Menu_OptionsAdvanced::Handle_Dismiss(void* obj, void* arg)
	{
		Menu_Options::Handle_Dismiss(obj, arg);
	}
}
