#pragma once

#include "IView.h"
#include "ScreenLock.h"

namespace Game
{
	class GaugeButton
	{
	public:
		void Setup(RectF rect, const AtlasImageMap* image, float min, float max, float value);
		
		void Update(float dt);
		void Render();
		void HandleTouch(Vec2F pos);
		
	private:
		void Clamp();
		
		RectF m_Rect;
		const AtlasImageMap* m_Image;
		float m_Min;
		float m_Max;
		float m_Value;
	};
	
	//
	
	class View_Options : public IView
	{
	public:
		View_Options();
		virtual ~View_Options();
		
		virtual void Initialize();
		
		void Show(View nextView);
		
	private:
		// --------------------
		// View
		// --------------------
		virtual void Render();
		virtual void Update(float dt);
		
		virtual int RenderMask_get();
		
		virtual float FadeTime_get();
		
		virtual void HandleFocus();
		virtual void HandleFocusLost();
		
		ScreenLock m_ScreenLock;
		
	public:
		View m_NextView;
	};
}
