#include "GameState.h"
#include "MenuMgr.h"
#include "TempRender.h"
#include "Textures.h"
#include "View_KeyBoard.h"
#include "VirtualKeyboard.h"

namespace Game
{
	View_KeyBoard::View_KeyBoard()
	{
		m_IsActive = false;
		
		m_KeyBoard = 0;
		m_Input = 0;
		
		m_NextView = View_Undefined;
	}
	
	View_KeyBoard::~View_KeyBoard()
	{
		delete m_KeyBoard;
		m_KeyBoard = 0;
		delete m_Input;
		m_Input = 0;
	}
	
	void View_KeyBoard::Initialize()
	{
		m_KeyBoard = 0;
		m_Input = new VirtualInput();
		
		m_BackDropTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
	}
	
	void View_KeyBoard::Show(const char* caption, const char* text, int maxLength, View nextView)
	{
		m_NextView = nextView;
		
		m_Input->Initialize(maxLength);
		m_Input->Caption_set(caption);
		m_Input->Text_set(text);
		
		g_GameState->ActiveView_set(::View_KeyBoard);
	}
	
	void View_KeyBoard::HandleFocus()
	{
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_Empty);
		
		m_IsActive = true;
		
		// activate keyboard
		
		CreateKeyboard();
		
		m_KeyBoard->Activate(1.0f);
		m_Input->Activate(0.5f);
		
		// start backdrop animation
		
		m_BackDropTimer.Start(AnimTimerMode_TimeBased, true, 1.0f, AnimTimerRepeat_None);
	}
	
	void View_KeyBoard::HandleFocusLost()
	{
		m_IsActive = false;
		
		// deactive keyboard
		
		m_KeyBoard->Deactivate(FadeTime_get());
		m_Input->Deactivate(FadeTime_get() / 2.0f);
		
		// start backdrop animation
		
		m_BackDropTimer.Start(AnimTimerMode_TimeBased, true, FadeTime_get(), AnimTimerRepeat_None);
	}
	
	void View_KeyBoard::Update(float dt)
	{
	}
	
	void View_KeyBoard::UpdateAnimation(float dt)
	{
		// update keyboard
		
		m_KeyBoard->Update(dt);
		
		// update text field
		
		m_Input->Update(dt);
	}
	
	void View_KeyBoard::Render()
	{
		// render backdrop
		
		float a;
		
		if (m_IsActive)
			a = 1.0f - m_BackDropTimer.Progress_get();
		else
			a = m_BackDropTimer.Progress_get();
		
		RenderRect(Vec2F(0.0f, 0.0f), Vec2F((float)VIEW_SX, (float)VIEW_SY), 0.0f, 0.0f, 0.0f, a, g_GameState->GetTexture(Textures::MENU_BANDIT_BACK));
		
		// render keyboard
		
		m_KeyBoard->Render();
		
		// render text field
		
		m_Input->Render();
	}
	
	int View_KeyBoard::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles | RenderMask_WorldBackground;
	}
	
	float View_KeyBoard::FadeTime_get()
	{
		return 0.5f;
	}
	
	void View_KeyBoard::CreateKeyboard()
	{
		delete m_KeyBoard;
		m_KeyBoard = 0;
		
		m_KeyBoard = new VirtualKeyBoard();
		
		m_KeyBoard->OnKeyPress = CallBack(m_Input, VirtualInput::HandleKeyPress);
		m_KeyBoard->OnReady = CallBack(this, HandleReady);
		m_KeyBoard->OnCancel = CallBack(this, HandleCancel);
	}
	
	// --------------------
	// Dialog
	// --------------------
	
	void View_KeyBoard::HandleReady(void* obj, void* arg)
	{
		View_KeyBoard* self = (View_KeyBoard*)obj;
		
		bool hasText = strlen(self->m_Input->ToString()) > 0;
		
		if (!hasText)
			return;
		
		self->m_ReturnCode = ReturnCode_Ok;
		
		g_GameState->ActiveView_set(self->m_NextView);
	}
	
	void View_KeyBoard::HandleCancel(void* obj, void* arg)
	{
		View_KeyBoard* self = (View_KeyBoard*)obj;
		
		self->m_ReturnCode = ReturnCode_Cancel;
		
		g_GameState->ActiveView_set(self->m_NextView);
	}
	
	View_KeyBoard::ReturnCode View_KeyBoard::ReturnCode_get() const
	{
		return m_ReturnCode;
	}
	
	std::string View_KeyBoard::Text_get() const
	{
		return m_Input->ToString();
	}

	void View_KeyBoard::Text_set(const char* text)
	{
		m_Input->Text_set(text);
	}
}
