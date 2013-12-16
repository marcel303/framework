#pragma once

#include "AnimTimer.h"
#include "IView.h"

namespace Game
{
	class View_Credits : public IView
	{
	public:
		View_Credits();
		void Initialize();
		
		// --------------------
		// View
		// --------------------
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		void Show(View nextView);
		
		View m_NextView;

		// --------------------
		// Credits
		// --------------------
		float CalcSpacing(int index);
		void Reset();
		
		int m_CreditsCount;
		float m_CreditsSize;
		
		// --------------------
		// Scrolling
		// --------------------
		float m_ScrollPosition;
		
		// --------------------
		// BBOS
		// --------------------
		float m_TextVisibility;

		// --------------------
		// Touch related
		// --------------------
		static bool HandleTouchBegin(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchMove(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchEnd(void* obj, const TouchInfo& touchInfo);
	};
}
