#pragma once

#include "Forward.h"
#include "IView.h"

namespace Game
{
	class View_KeyBoard : public IView
	{
	public:
		enum ReturnCode
		{
			ReturnCode_Ok,
			ReturnCode_Cancel
		};
		
		View_KeyBoard();
		virtual ~View_KeyBoard();
		
		virtual void Initialize();
		
		void Show(const char* caption, const char* text, int maxLength, View nextView);
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		virtual void Update(float dt);
		virtual void UpdateAnimation(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		virtual float FadeTime_get();
		
	private:
		void CreateKeyboard();
		
		VirtualKeyBoard* m_KeyBoard;
		VirtualInput* m_Input;
		
		// --------------------
		// Dialog
		// --------------------
		View m_NextView;
		ReturnCode m_ReturnCode;
		
		static void HandleReady(void* obj, void* arg);
		static void HandleCancel(void* obj, void* arg);
		
	public:
		ReturnCode ReturnCode_get() const;
		std::string Text_get() const;
		void Text_set(const char* text);
		
	private:
		// --------------------
		// Animation
		// --------------------
		AnimTimer m_BackDropTimer;
		bool m_IsActive;
	};
}
