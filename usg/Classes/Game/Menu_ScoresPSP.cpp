#include "GameState.h"
#include "GuiButton.h"
#include "GuiListSlider.h"
#include "Menu_ScoresPSP.h"
#include "MenuRender.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "View_ScoresPSP.h"

namespace GameMenu
{
	const float BUTTON_SIZE = 30.0f;

	static DesignInfo design[] =
	{
		DesignInfo("gamemode_prev",   false, Vec2F(90.0f - BUTTON_SIZE, 210.0f ), Vec2F()),
		DesignInfo("gamemode_next",   false, Vec2F(VIEW_SX - 90.0f, 210.0f),      Vec2F()),
		DesignInfo("difficulty_prev", false, Vec2F(90.0f - BUTTON_SIZE, 230.0f),  Vec2F()),
		DesignInfo("difficulty_next", false, Vec2F(VIEW_SX - 90.0f, 230.0f),      Vec2F()),
		DesignInfo("gamemode",        true,  Vec2F(90.0f, 200.0f),                Vec2F(VIEW_SX - 90.0f * 2.0f, 20.0f)),
		DesignInfo("difficulty",      true,  Vec2F(90.0f, 220.0f),                Vec2F(VIEW_SX - 90.0f * 2.0f, 20.0f)),
		DesignInfo("back",            false, Vec2F(20.0f, VIEW_SY - 50.0f),       Vec2F())
	};

	const GuiListItem sGameModeNames[] =
	{
		GuiListItem("classic", 0),
		GuiListItem()
	};
	const GuiListItem sClassicDifficultyNames[] =
	{
		GuiListItem("easy", Difficulty_Easy),
		GuiListItem("hard", Difficulty_Hard),
		GuiListItem("custom", Difficulty_Custom),
		GuiListItem()
	};

	void Menu_ScoresPSP::Init()
	{
		SetTransition(TransitionEffect_Slide, Vec2F(0.0f, 100.0f), 0.3f);
		
		AddButton(Button::Make_Shape("gamemode_prev",   Vec2F(), g_GameState->GetShape(Resources::POWERUP_CREDITS_SMALL), 0, CallBack(this, Handle_GameMode_Prev)));
		AddButton(Button::Make_Shape("gamemode_next",   Vec2F(), g_GameState->GetShape(Resources::POWERUP_CREDITS_SMALL), 0, CallBack(this, Handle_GameMode_Next)));
		AddButton(Button::Make_Shape("difficulty_prev", Vec2F(), g_GameState->GetShape(Resources::POWERUP_CREDITS_SMALL), 0, CallBack(this, Handle_Difficulty_Prev)));
		AddButton(Button::Make_Shape("difficulty_next", Vec2F(), g_GameState->GetShape(Resources::POWERUP_CREDITS_SMALL), 0, CallBack(this, Handle_Difficulty_Next)));

		AddElement(new GuiListSlider("gamemode",   sGameModeNames,          Resources::FONT_USUZI_SMALL, true, 0, CallBack(this, Handle_GameModeChanged  )));
		AddElement(new GuiListSlider("difficulty", sClassicDifficultyNames, Resources::FONT_USUZI_SMALL, true, 0, CallBack(this, Handle_DifficultyChanged)));

		AddButton(Button::Make_Shape("back", Vec2F(), g_GameState->GetShape(Resources::BUTTON_BACK), 0, CallBack(this, Handle_GoBack)));

		DESIGN_APPLY(this, design);
	}

	void Menu_ScoresPSP::HandleFocus()
	{
		Menu::HandleFocus();

		// reset gamemode/difficulty

		FindListSlider("gamemode")->Value_set(0, false);
		FindListSlider("difficulty")->Value_set(0, false);

		RefreshScoreView();
	}

	void Menu_ScoresPSP::HandleBack()
	{
		Handle_GoBack(this, 0);
	}

	bool Menu_ScoresPSP::Render()
	{
		if (!Menu::Render())
			return false;

		return true;
	}

	void Menu_ScoresPSP::RefreshScoreView()
	{
#if defined(PSP_UI)
		Game::View_ScoresPSP* view = (Game::View_ScoresPSP*)g_GameState->GetView(View_ScoresPSP);

		int gameMode = FindListSlider("gamemode")->Curr_get()->mTag;
		Difficulty difficulty = (Difficulty)FindListSlider("difficulty")->Curr_get()->mTag;

		view->Show(gameMode, difficulty);
#endif
	}

	void Menu_ScoresPSP::Handle_GameMode_Prev(void* obj, void* arg)
	{
		Menu_ScoresPSP* self = (Menu_ScoresPSP*)obj;

		if (self->FindListSlider("gamemode")->SelectPrev())
		{
			self->RefreshScoreView();
		}
	}

	void Menu_ScoresPSP::Handle_GameMode_Next(void* obj, void* arg)
	{
		Menu_ScoresPSP* self = (Menu_ScoresPSP*)obj;

		if (self->FindListSlider("gamemode")->SelectNext())
		{
			self->RefreshScoreView();
		}
	}

	void Menu_ScoresPSP::Handle_Difficulty_Prev(void* obj, void* arg)
	{
		Menu_ScoresPSP* self = (Menu_ScoresPSP*)obj;

		if (self->FindListSlider("difficulty")->SelectPrev())
		{
			self->RefreshScoreView();
		}
	}

	void Menu_ScoresPSP::Handle_Difficulty_Next(void* obj, void* arg)
	{
		Menu_ScoresPSP* self = (Menu_ScoresPSP*)obj;

		if (self->FindListSlider("difficulty")->SelectNext())
		{
			self->RefreshScoreView();
		}
	}

	void Menu_ScoresPSP::Handle_GoBack(void* obj, void* arg)
	{
		g_GameState->ActiveView_set(View_Main);
	}

	void Menu_ScoresPSP::Handle_GameModeChanged(void* obj, void* arg)
	{
		Menu_ScoresPSP* self = (Menu_ScoresPSP*)obj;

		self->RefreshScoreView();
	}

	void Menu_ScoresPSP::Handle_DifficultyChanged(void* obj, void* arg)
	{
		Menu_ScoresPSP* self = (Menu_ScoresPSP*)obj;

		self->RefreshScoreView();
	}
}
