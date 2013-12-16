#pragma once

#include "AnimTimer.h"
#include "Forward.h"
#include "IView.h"

namespace Game
{
	/*
	 
	 - dark background
	 
	 - stats hexagon
	 
	 - eerie sounds
	 
	 - background scrolling
	 
	 - grid_effect base hue scare tractics
	 
	 - boss name
	 
	 - bandit level (evolution)
	 
	 */
	class View_BanditIntro : public IView
	{
	public:
		View_BanditIntro(int screenScale);
		virtual ~View_BanditIntro();
		void Initialize();
		
	private:
		// --------------------
		// View
		// --------------------
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		virtual float FadeTime_get();
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		// --------------------
		// State
		// --------------------
		bool m_IsActive;
		
		// --------------------
		// Boss preview
		// --------------------
		void SetupMaxiPreview();
		
		Bandits::EntityBandit* m_MaxiPreview;
		
		// --------------------
		// Animation
		// --------------------
		float AnimationOffset_get();
		void GetScannerAngles(float& oAngle1, float& oAngle2);
		
		AnimTimer m_BackDropTimer;
		AnimTimer m_SlideTimer;
		AnimTimer m_ProgressSavedTimer;
		AnimTimer m_TouchToContinueTimer;
		
		// --------------------
		// Drawing
		// --------------------
		int m_ScreenScale;
		const AtlasImageMap* m_BackDrop;
		const char* m_EncouragementText;
		float m_EncouragementTextSx;
		
		void UpdateBanditInfo();
		
		void RenderBackground();
		void RenderPrimary();
		void RenderMaxiPreview();
		void RenderBanner(Vec2F offset, float scroll);
		void RenderScroller(Vec2F pos, float scroll);
		void RenderBanditInfo();
	};
}
