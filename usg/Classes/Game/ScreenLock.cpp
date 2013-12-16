#include "GameState.h"
#include "ScreenLock.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"

#ifdef PSP_UI
#define LOCK_SY 20.0f
#define FONT Resources::FONT_USUZI_SMALL
#define BOUNCE_ACCEL -100.0f
#define BOUNCE_FALLOFF 0.5f
#define BOUNCE_INITSPEED -250.0f
#define BOUNCE_COLLISION 0.2f
#else
#define LOCK_SY 40.0f
#define FONT Resources::FONT_CALIBRI_LARGE
#define BOUNCE_ACCEL 0.0f
#define BOUNCE_FALLOFF 0.0f
#define BOUNCE_INITSPEED 0.0f
#define BOUNCE_COLLISION 0.0f
#endif
#define FADE_TIME 0.25f

namespace Game
{
	ScreenLock::ScreenLock()
	{
		m_Text = 0;
		m_AltMode = false;
		m_BounceSpeed = 0.0f;
		m_BouncePos = 0.0f;
	}
	
	void ScreenLock::Initialize(const char* text, bool altMode)
	{
		m_Text = text;
		m_AltMode = altMode;
		m_BounceSpeed = 0.0f;
		m_BouncePos = 0.0f;
	
		m_LockAnim.Initialize(g_GameState->m_TimeTracker_Global, false);
	}
	
	void ScreenLock::Initialize(const char* text)
	{
		Initialize(text, false);
	}
	
	void ScreenLock::Start(bool isActive)
	{
		m_IsActive = isActive;
		
		if (m_IsActive)
			m_LockAnim.Start(AnimTimerMode_TimeBased, true, FADE_TIME, AnimTimerRepeat_None);
		else
			m_LockAnim.Start(AnimTimerMode_TimeBased, true, FADE_TIME, AnimTimerRepeat_None);

#ifdef PSP_UI
		if (m_IsActive)
		{
			m_BounceSpeed = BOUNCE_INITSPEED;
			m_BouncePos = 100.0f;
		}
#endif
	}
	
	float ScreenLock::FadeTime_get() const
	{
		return FADE_TIME;
	}
	
	// --------------------
	// Animation
	// --------------------
	
	void ScreenLock::Update()
	{
		float t = m_LockAnim.Progress_get();
		
		if (!m_IsActive)
			t = 1.0f - t;
		
		m_LockPos[0] = - t * LOCK_SY;
		m_LockPos[1] = VIEW_SY - (1.0f - t) * LOCK_SY;

		const float dt = 1.0f / 60.0f; // fixme..

		m_BounceSpeed += BOUNCE_ACCEL * dt;
		m_BounceSpeed *= powf(BOUNCE_FALLOFF, dt);
		m_BouncePos += m_BounceSpeed * dt;
		if (m_BouncePos < 0.0f)
		{
			m_BouncePos *= -1.0f;
			m_BounceSpeed *= -BOUNCE_COLLISION;
		}
	}
	
	void ScreenLock::Render()
	{
		Update();
		
		//
	
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::MENU_COLOR_WHITE);
		
		Vec2F size((float)VIEW_SX, (float)LOCK_SY);

//#if defined(PSP) || defined(DEBUG)
#if 0
		//const float c = (Calc::Sin_Fast(g_TimerRT.Time_get()) + 1.0f) * 0.5f * 0.2f;

		RenderRect(Vec2F(0.0f, m_LockPos[0]), size, 0.0f, 0.0f, 0.0f, 1.0f, image);
		RenderRect(Vec2F(0.0f, m_LockPos[1]), size, 0.0f, 0.0f, 0.0f, 1.0f, image);
#else
		const Vec2F px1(0.0f, m_LockPos[0]);
		const Vec2F px2(0.0f, m_LockPos[1]);
		const Vec2F p0(m_BouncePos, m_LockPos[0]);
		const Vec2F p1(-m_BouncePos, m_LockPos[1]);

		if (m_AltMode == false)
		{
			RenderRect(px1, size, 0.0f, 0.0f, 0.0f, 1.0f, image);
			RenderText(p0, size, g_GameState->GetFont(FONT), SpriteColors::White, TextAlignment_Left, TextAlignment_Center, false, m_Text);
			RenderRect(px2, size, 0.0f, 0.0f, 0.0f, 1.0f, image);
			RenderText(p1, size, g_GameState->GetFont(FONT), SpriteColors::White, TextAlignment_Right, TextAlignment_Center, false, m_Text);
		}
		else
		{
			RenderRect(px1, size, 0.0f, 0.0f, 0.0f, 1.0f, image);
			RenderText(p1, size, g_GameState->GetFont(FONT), SpriteColors::White, TextAlignment_Right, TextAlignment_Center, false, m_Text);
			RenderRect(px2, size, 0.0f, 0.0f, 0.0f, 1.0f, image);
		}
#endif
	}
}
