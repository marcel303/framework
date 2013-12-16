#include "GameRound.h"
#include "GameState.h"
#include "Mat3x2.h"
#include "TempRender.h"
#include "Textures.h"
#include "UiCriticalIndicator.h"
#include "UsgResources.h"

#define ZOOM_SPEED 2.0f

#if defined(PSP_UI)
const float dy = 14.0f;
#define ZOOM0_POS Vec2F(VIEW_SX/2.0f-60.0f, VIEW_SY-30.0f)
#define ZOOM1_POS Vec2F(VIEW_SX/2.0f-80.0f, VIEW_SY-30.0f-dy/2.0f)
#define ZOOM0_SIZE Vec2F(120.0f, 10.0f)
#define ZOOM1_SIZE Vec2F(160.0f, 10.0f+dy)
#else
#define ZOOM0_POS Vec2F(VIEW_SX/2.0f-60.0f, 30.0f)
#define ZOOM1_POS Vec2F(VIEW_SX/2.0f-80.0f, 30.0f)
#define ZOOM0_SIZE Vec2F(120.0f, 10.0f)
#define ZOOM1_SIZE Vec2F(160.0f, 25.0f)
#endif

namespace Game
{
	UiCriticalGauge::UiCriticalGauge()
	{
		Initialize();
	}
	
	void UiCriticalGauge::Initialize()
	{
		m_IsZoomed = false;
		m_Zoom = 0.0f;
		m_Value = 0.0f;
	}
	
	void UiCriticalGauge::Setup()
	{
		Initialize();
	}
	
	void UiCriticalGauge::Update(float dt)
	{
		UpdateZoom(dt);
		UpdateValue(dt);
	}
	
	void UiCriticalGauge::Render()
	{
		RenderGauge();
		
		//if (IsZoomComplete_get())
		//{
			RenderText();
		//}
	}
	
	void UiCriticalGauge::EnableZoom(float duration)
	{
		m_ZoomDisableTrigger.Start(duration);
		
		m_IsZoomed = true;
	}
	
	void UiCriticalGauge::UpdateZoom(float dt)
	{
		if (m_ZoomDisableTrigger.Read())
		{
			m_IsZoomed = false;
		}
		
		if (m_IsZoomed)
		{
			m_Zoom = Calc::Mid(m_Zoom + ZOOM_SPEED * dt, 0.0f, 1.0f);
		}
		else
		{
			m_Zoom = Calc::Mid(m_Zoom - ZOOM_SPEED * dt, 0.0f, 1.0f);
		}
	}
	
	void UiCriticalGauge::UpdateValue(float dt)
	{
		float v1 = 1.0f - powf(0.1f, dt);
		float v2 = 1.0f - v1;
		
		float value = GaugeValue_get();
		
		m_Value = value * v1 + m_Value * v2;
	}
	
	bool UiCriticalGauge::IsZoomComplete_get() const
	{
		return m_Zoom == 1.0f;
	}
	
	void UiCriticalGauge::RenderGauge()
	{
		Vec2F pos = ZOOM0_POS.LerpTo(ZOOM1_POS, m_Zoom);
		Vec2F size = ZOOM0_SIZE.LerpTo(ZOOM1_SIZE, m_Zoom);
		
		RenderRect(pos, size, 0.0f, 0.0f, 0.0f, g_GameState->m_GameSettings->m_HudOpacity, g_GameState->GetTexture(Textures::COLOR_WHITE));
		
		float borderSize = Calc::Lerp(2.0f, 3.0f, m_Zoom);
		Vec2F border(borderSize, borderSize);
		
		pos += border;
		size -= border * 2.0f;
		
		//size = size ^ Vec2F(m_Value, 1.0f);
		
		if (m_Value > 0.0f)
		{
//			RenderRect(pos, size, 255 / 255.0f, 0 / 255.0f, 215 / 255.0f, 1.0f, g_GameState->GetTexture(Textures::COLOR_WHITE));
			
			DrawBarH(*g_GameState->m_SpriteGfx, pos, pos + size, g_GameState->GetTexture(Textures::INDICATOR_CRITICAL), m_Value, SpriteColor_MakeF(1.f, 1.f, 1.f, g_GameState->m_GameSettings->m_HudOpacity).rgba);
			
			if (m_Value >= 0.95f)
			{
				//float alpha = (sinf(g_GameState->m_TimeTracker_World.Time_get() * 6.0f) + 1.0f) * 0.5f;
				
				//RenderRect(pos, size, 0.5f, 0.0f, 1.0f, alpha, g_GameState->GetTexture(Textures::COLOR_WHITE));
			}
			
			if (m_Zoom > 0.0f)
			{
				//float alpha = sinf(m_Zoom * Calc::mPI) * 0.5f;
				
//				RenderRect(pos, size, 1.0f, 1.0f, 1.0f, alpha, g_GameState->GetTexture(Textures::COLOR_WHITE));
			}
		}
	}
	
	void UiCriticalGauge::RenderText()
	{
		if (m_Zoom == 0.0f)
			return;

		const float scale = Calc::Saturate(m_Zoom * 2.0f);
		Mat3x2 mat;
		mat.MakeScaling(scale, scale);
		Vec2F pos = ZOOM0_POS.LerpTo(ZOOM1_POS, m_Zoom);
		Vec2F size = ZOOM0_SIZE.LerpTo(ZOOM1_SIZE, m_Zoom);
		pos = pos + size / 2.0f;
		mat.SetTrans(Vec2F(Calc::RoundDown(pos[0]), Calc::RoundDown(pos[1])));

		::RenderText(mat, g_GameState->GetFont(Resources::FONT_USUZI_SMALL), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, "CRITICAL");
	}
	
	float UiCriticalGauge::GaugeValue_get() const
	{
		int wave = g_GameState->m_GameRound->Classic_Wave_get();
		int waveCount = g_GameState->m_GameRound->Classic_WaveCount_get();
			
		if (g_GameState->m_GameRound->Classic_RoundState_get() == RoundState_PlayMaxiBoss)
			return 1.0f;
		else
			return (wave + 1) / (waveCount + 1.0001f);
	}
}
