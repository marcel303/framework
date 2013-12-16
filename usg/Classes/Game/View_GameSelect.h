#pragma once

#include "AngleController.h"
#include "IView.h"
#include "ScreenLock.h"
#include "TriggerTimerEx.h"

#ifdef IPAD
	#define GAMESELECT_FANCY_SELECT 0
#else
	#define GAMESELECT_FANCY_SELECT 0
#endif

namespace Game
{
	class ZoomAnim
	{
	public:
		void Setup(RectF src, RectF dst);
		
		RectF Interpolate(float t);
		
	private:
		RectF mSrc;
		RectF mDst;
	};
	
	class View_GameSelect : public IView
	{
	public:
		View_GameSelect();
		void Initialize();
		virtual ~View_GameSelect();
		
	private:
		// --------------------
		// View
		// --------------------
		virtual void Update(float dt);
		virtual void Render();
		
		virtual int RenderMask_get();
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		virtual float FadeTime_get();
		
		ScreenLock m_ScreenLock;
		bool m_IsActive;
		
		// --------------------
		// Screens
		// --------------------
	private:
		void DestroyTexture(int textureIndex);
		void CreateTexture(int index, int textureIndex);
		int FindTexture(int index);
		
	private:
		AnimTimer m_FlashTimer;
		AnimTimer m_GlowTimer;
		
		// slides
		ZoomAnim CreateSlideAnim();
	public:
		void NextSlide(bool useFlash);
	private:
		void UpdateSlide(float dt);
		
		ZoomAnim m_SlideAnim[2];
		TriggerTimerG m_SlideProgress[2];
		TriggerTimerG m_SlideTrigger;
		Res* m_Texture[2];
		float m_SlideOpacity;
		int m_SlideIndex;
		
#if GAMESELECT_FANCY_SELECT
		// --------------------
		// Touch related
		// --------------------
		static bool HandleTouchBegin(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchMove(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchEnd(void* obj, const TouchInfo& touchInfo);
		
		// Select
		void UpdateSelect(float dt);
		void UpdateSelectIndex(float movementDirection);
		void RenderSelect();
		void SelectMoveBegin();
		void SelectMoveEnd(Vec2F position);
		void SelectMoveUpdate(float d);
		void SelectPosition_set(float v);
		int SelectIndex_get() const;
		void RenderGlyph(float offset, int idx, bool isCenterGlyph);
		
		AngleController m_SelectController;
		float m_SelectPosition;
		float m_SelectPositionD;
		bool m_SelectActive;
		int m_SelectIndex;
		bool m_SelectHasMoved;
		float m_SelectGlowTime;
		bool m_SelectIsAnimating;
		int m_SelectedIndex;
		
		bool m_DifficultyOverActive;
#endif
	};
}
