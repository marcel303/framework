#include "GameSettings.h"
#include "GameState.h"
#include "Graphics.h"
#include "MenuMgr.h"
#include "ResIO.h"
#include "SoundEffectMgr.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "TouchDLG.h"
#include "UsgResources.h"
#include "View_GameSelect.h"

#define MAX_SCREENS 4

#define SLIDE_OPAC 1.5f
#define SLIDE_NEXT 4.0f
#define SLIDE_TIME (SLIDE_OPAC + SLIDE_NEXT)

#define SELECT_SIZE 160.0f
#define SELECT_COUNT 3
#define SELECT_SPEED 1.6f
#define SELECT_HISTORY 2
#define SELECT_GLOW_TIME 0.3f
#define SELECT_CENTER Vec2F(VIEW_SX/2.0f, VIEW_SY/2.0f)

#if GAMESELECT_FANCY_SELECT
const static Vec2F DIFFICULTY_BUTTON_SIZE = Vec2F(VIEW_SX, 80.0f);
const static float DIFFICULTY_BUTTON_SPACING = 100.0f;
const static Vec2F DIFFICULTY_BUTTON_POS_EASY = Vec2F(0.0f, VIEW_SY/2.0f - DIFFICULTY_BUTTON_SPACING/2.0f - DIFFICULTY_BUTTON_SIZE[1]/2.0f);
const static Vec2F DIFFICULTY_BUTTON_POS_HARD = Vec2F(0.0f, VIEW_SY/2.0f + DIFFICULTY_BUTTON_SPACING/2.0f - DIFFICULTY_BUTTON_SIZE[1]/2.0f);
const static RectF DIFFICULTY_BUTTON_RECT_EASY(DIFFICULTY_BUTTON_POS_EASY, DIFFICULTY_BUTTON_SIZE);
const static RectF DIFFICULTY_BUTTON_RECT_HARD(DIFFICULTY_BUTTON_POS_HARD, DIFFICULTY_BUTTON_SIZE);
#endif

namespace Game
{
	void ZoomAnim::Setup(RectF src, RectF dst)
	{
		mSrc = src;
		mDst = dst;
	}
	
	RectF ZoomAnim::Interpolate(float t)
	{
		t = Calc::Mid(t, 0.0f, 1.0f);
		
		Vec2F position = mSrc.m_Position.LerpTo(mDst.m_Position, t);
		Vec2F size = mSrc.m_Size.LerpTo(mDst.m_Size, t);
		
		return RectF(position, size);
	}

	//
	
	View_GameSelect::View_GameSelect()
	{
	}
	
	View_GameSelect::~View_GameSelect()
	{
	}
	
	void View_GameSelect::Initialize()
	{
		m_ScreenLock.Initialize("", true);
		
		// screens
		
//		m_ScreenIndex = 0;
//		m_Texture = 0;
		m_FlashTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		
		m_GlowTimer.Initialize(g_GameState->m_TimeTracker_Global, false);
		
		// slides
		
		m_SlideAnim[0] = CreateSlideAnim();
		m_SlideAnim[1] = CreateSlideAnim();
		
		m_SlideTrigger.Start(0.0f);
		m_Texture[0] = 0;
		m_Texture[1] = 0;
		m_SlideOpacity = 0.0f;
		m_SlideIndex = 0;
		
#if GAMESELECT_FANCY_SELECT
		// touch related
		
		TouchListener listener;
		listener.Setup(this, HandleTouchBegin, HandleTouchEnd, HandleTouchMove);
		g_GameState->m_TouchDLG->Register(USG::TOUCH_PRIO_GAMESELECT, listener);
		
		m_DifficultyOverActive = false;
#endif
	}
	
	void View_GameSelect::HandleFocus()
	{
		m_IsActive = true;
		
		g_GameState->Interface_get()->ActiveMenu_set(GameMenu::MenuType_GameSelect);
#if GAMESELECT_FANCY_SELECT
		g_GameState->m_TouchDLG->Enable(USG::TOUCH_PRIO_GAMESELECT);
#endif
		
		m_ScreenLock.Start(true);
		
//		m_ScreenIndex = Calc::Random() % MAX_SCREENS;
		m_SlideIndex = Calc::Random() % MAX_SCREENS;
		
		m_GlowTimer.Start(AnimTimerMode_TimeBased, false, 0.85f, AnimTimerRepeat_Mirror);
		
//		NextScreen();
		NextSlide(false);
		NextSlide(false);
		
#if GAMESELECT_FANCY_SELECT
		m_SelectPosition = 0.0f;
		m_SelectPositionD = 0.0f;
		m_SelectActive = false;
		m_SelectIndex = 0;
		m_SelectGlowTime = 0.0f;
		m_SelectHasMoved = false;
		m_SelectIsAnimating = false;
		m_SelectedIndex = -1;
		m_SelectController.Setup(m_SelectPosition, m_SelectPosition, SELECT_SPEED);
		
		m_DifficultyOverActive = false;
#endif
	}
	
	void View_GameSelect::HandleFocusLost()
	{
		DestroyTexture(0);
		DestroyTexture(1);
		
		m_IsActive = false;
		
		m_ScreenLock.Start(false);
		
#if GAMESELECT_FANCY_SELECT
		g_GameState->m_TouchDLG->Disable(USG::TOUCH_PRIO_GAMESELECT);
#endif
		
		Assert(m_Texture[0] == 0);
		Assert(m_Texture[1] == 0);
	}
	
	float View_GameSelect::FadeTime_get()
	{
		return m_ScreenLock.FadeTime_get();
	}
	
	void View_GameSelect::Update(float dt)
	{
		UpdateSlide(dt);
		
#if GAMESELECT_FANCY_SELECT
		UpdateSelect(dt);
#endif
	}
	
	void View_GameSelect::Render()
	{
		if (m_IsActive)
		{
			Assert(m_Texture[0] && m_Texture[0]->device_data);
			Assert(m_Texture[1] && m_Texture[1]->device_data);
			
			// flush all pending output
			
			g_GameState->m_SpriteGfx->Flush();
			
			//gGraphics.Clear(0.0f, 0.2f, 0.3f, 0.0f);
			
			// draw stuff
			
			SpriteColor color = SpriteColor_ScaleF(SpriteColors::HitEffect, m_FlashTimer.Progress_get());
			//color = SpriteColors::White;
			m_FlashTimer.Tick();
			
#if defined(IPAD) || defined(WIN32) || defined(BBOS)
			float tsx = 1024.0f;
			float tsy = 512.0f;
			float scaleX = VIEW_SX / tsx;
			float scaleY = VIEW_SY / tsy;
			float py = 0.0f;
#else
			float tsx = 512.0f;
			float tsy = 256.0f;
			float scaleX = VIEW_SX / 320.0f;
			float scaleY = VIEW_SY / 240.0f;
			
			float py = (VIEW_SY - tsy * scaleY) / 2.0f;
#endif
			
#if 0
			DrawRect(g_GameState->m_SpriteGfx, 0.0f, py, 512.0f, 256.0f, 0.0f, 0.0f, 1.0f, 1.0f, color.rgba);
			g_GameState->m_SpriteGfx.Flush();
#else
			RectF rect[2];
			rect[0] = m_SlideAnim[0].Interpolate(m_SlideProgress[0].Progress_get());
			rect[1] = m_SlideAnim[1].Interpolate(m_SlideProgress[1].Progress_get());
			
			//

			color.v[3] = Calc::Mid((int)(m_SlideOpacity * 255.0f), 0, 255);

			if (color.v[3] != 255)
			{
				gGraphics.TextureSet(m_Texture[0]);
				gGraphics.BlendModeSet(BlendMode_Normal_Opaque_Add);

				DrawRect(*g_GameState->m_SpriteGfx, 0.0f, py, tsx * scaleX, tsy * scaleY, rect[0].m_Position[0], rect[0].m_Position[1], rect[0].m_Size[0], rect[0].m_Size[1], color.rgba);
				g_GameState->m_SpriteGfx->Flush();
			}

			//

			gGraphics.TextureSet(m_Texture[1]);

			if (color.v[3] != 255)
				gGraphics.BlendModeSet(BlendMode_Normal_Transparent_Add);
			else
				gGraphics.BlendModeSet(BlendMode_Normal_Opaque_Add);

			DrawRect(*g_GameState->m_SpriteGfx, 0.0f, py, tsx * scaleX, tsy * scaleY, rect[1].m_Position[0], rect[1].m_Position[1], rect[1].m_Size[0], rect[1].m_Size[1], color.rgba);
			g_GameState->m_SpriteGfx->Flush();
#endif
			
			// restore draw state
			gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
			gGraphics.TextureSet(g_GameState->m_TextureAtlas[g_GameState->m_ActiveDataSet]->m_Texture);
			
#if GAMESELECT_FANCY_SELECT
			if (m_SelectGlowTime > 0.0f)
				gGraphics.BlendModeSet(BlendMode_Normal_Transparent_Add);
			else
				gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
			
			RenderSelect();
			
			g_GameState->m_SpriteGfx->Flush();
#endif

			// restore draw states
			gGraphics.BlendModeSet(BlendMode_Normal_Transparent);
			gGraphics.TextureSet(g_GameState->m_TextureAtlas[g_GameState->m_ActiveDataSet]->m_Texture);
			
#if GAMESELECT_FANCY_SELECT
			if (m_DifficultyOverActive)
			{
				RenderRect(
					Vec2F(0.0f, 0.0f), Vec2F(VIEW_SX, VIEW_SY),
					0.0f, 0.0f, 0.0f, 0.7f,
					g_GameState->GetTexture(Textures::COLOR_WHITE));
				
				RenderRect(
					DIFFICULTY_BUTTON_POS_EASY,
					DIFFICULTY_BUTTON_SIZE,
					1.0f, 0.0f, 0.0f, 0.7f,
					g_GameState->GetTexture(Textures::MINI_MAGNET_BEAM));
				RenderText(
					DIFFICULTY_BUTTON_POS_EASY,
					DIFFICULTY_BUTTON_SIZE,
					g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, "Easy");
				RenderRect(
					   DIFFICULTY_BUTTON_POS_HARD,
					   DIFFICULTY_BUTTON_SIZE,
					0.0f, 0.0f, 1.0f, 0.7f,
					g_GameState->GetTexture(Textures::MINI_MAGNET_BEAM));
				RenderText(
					DIFFICULTY_BUTTON_POS_HARD,
					DIFFICULTY_BUTTON_SIZE,
					g_GameState->GetFont(Resources::FONT_CALIBRI_LARGE), SpriteColors::White, TextAlignment_Center, TextAlignment_Center, true, "Hard");
			}
#endif
		}
		
		m_ScreenLock.Render();
	}
	
	int View_GameSelect::RenderMask_get()
	{
		return RenderMask_Interface | RenderMask_Particles;
	}
	
	// --------------------
	// Screens
	// --------------------
	
	void View_GameSelect::DestroyTexture(int textureIndex)
	{
		if (m_Texture[textureIndex] == 0)
			return;
		
#if !defined(PSP) && !defined(IPAD) && !defined(WIN32) && !defined(LINUX) && !defined(MACOS) && !defined(BBOS)
		gGraphics.TextureDestroy(m_Texture[textureIndex]);
#endif

		m_Texture[textureIndex] = 0;
	}
	
	void View_GameSelect::CreateTexture(int index, int textureIndex)
	{
		Assert(m_Texture[textureIndex] == 0);

		int textureId = FindTexture(index);
		
		m_Texture[textureIndex] = g_GameState->m_ResMgr.Get(textureId);
		
#if !defined(PSP) && !defined(IPAD) && !defined(WIN32) && !defined(LINUX) && !defined(MACOS) && !defined(BBOS)
		gGraphics.TextureCreate(m_Texture[textureIndex]);
#endif
	}
	
	int View_GameSelect::FindTexture(int index)
	{
		switch (index)
		{
			case 0:
				return Resources::SLIDE_01;
			case 1:
				return Resources::SLIDE_02;
			case 2:
				return Resources::SLIDE_03;
			case 3:
				return Resources::SLIDE_04;
			default:
#ifndef DEPLOYMENT
				throw ExceptionNA();
#else
				return Resources::SLIDE_01;
#endif
		}
	}
	
	ZoomAnim View_GameSelect::CreateSlideAnim()
	{
		float scale = Calc::Random(0.6f, 0.9f);
		float scaleInv = 1.0f - scale;
		float posX = Calc::Random(0.0f, scaleInv);
		float posY = Calc::Random(0.0f, scaleInv);
		
		RectF src(Vec2F(posX, posY), Vec2F(scale, scale));
		RectF dst(Vec2F(0.0f, 0.0f), Vec2F(1.0f, 1.0f));
		
		if (Calc::Random(0, 1))
		{
			std::swap(src, dst);
		}
		
		ZoomAnim anim;
		
		anim.Setup(src, dst);
		
		return anim;
	}
	
	void View_GameSelect::NextSlide(bool useFlash)
	{
		m_SlideIndex = (m_SlideIndex + 1) % MAX_SCREENS;
		
		std::swap(m_SlideAnim[0], m_SlideAnim[1]);
		std::swap(m_Texture[0], m_Texture[1]);
		std::swap(m_SlideProgress[0], m_SlideProgress[1]);

		DestroyTexture(1);
		CreateTexture(m_SlideIndex, 1);
		m_SlideAnim[1] = CreateSlideAnim();
		
		m_SlideTrigger.Start(SLIDE_NEXT);
		m_SlideProgress[1].Start(SLIDE_TIME);
		m_SlideOpacity = 0.0f;
		
		if (useFlash)
		{
			m_FlashTimer.Start(AnimTimerMode_FrameBased, true, 15, AnimTimerRepeat_None);
			m_ScreenLock.Start(true);
		}
	}
	
	void View_GameSelect::UpdateSlide(float dt)
	{
		//m_SlideOpacity += dt / SLIDE_OPAC;
		m_SlideOpacity = m_SlideTrigger.Progress_get() * SLIDE_NEXT / SLIDE_OPAC;
		
		if (m_SlideTrigger.Read())
		{
			NextSlide(false);
		}
	}
	
#if GAMESELECT_FANCY_SELECT
	// --------------------
	// Touch related
	// --------------------
	
	bool View_GameSelect::HandleTouchBegin(void* obj, const TouchInfo& touchInfo)
	{
		View_GameSelect* self = (View_GameSelect*)obj;
		
		if (self->m_DifficultyOverActive)
		{
			GameMode gameMode = GameMode_ClassicPlay;
			
			switch (self->m_SelectedIndex)
			{
			case 0:
				gameMode = GameMode_ClassicLearn;
				break;
			case 1:
				gameMode = GameMode_ClassicPlay;
				break;
			case 2:
				gameMode = GameMode_InvadersPlay;
				break;
			}
			
			if (DIFFICULTY_BUTTON_RECT_EASY.IsInside(touchInfo.m_LocationView))
			{
				g_GameState->GameBegin(gameMode, Difficulty_Easy, false);
				return true;
			}
			if (DIFFICULTY_BUTTON_RECT_HARD.IsInside(touchInfo.m_LocationView))
			{
				g_GameState->GameBegin(gameMode, Difficulty_Hard, false);
				return true;
			}
			
			return true;
		}
		else if (self->m_SelectActive)// || self->m_SelectedIndex >= 0)
			return false;
		else
		{
			self->SelectMoveBegin();
			return true;
		}
	}
	
	bool View_GameSelect::HandleTouchMove(void* obj, const TouchInfo& touchInfo)
	{
		View_GameSelect* self = (View_GameSelect*)obj;
		
		if (self->m_DifficultyOverActive)
		{
			// nop
		}
		else
		{
			float d = touchInfo.m_LocationDelta[0] / SELECT_SIZE;
			
			self->SelectMoveUpdate(d);
		}
		
		return true;
	}
	
	bool View_GameSelect::HandleTouchEnd(void* obj, const TouchInfo& touchInfo)
	{
		View_GameSelect* self = (View_GameSelect*)obj;
		
		if (self->m_DifficultyOverActive)
		{
			// nop
		}
		else
		{
			self->SelectMoveEnd(touchInfo.m_LocationView);
		}
		
		return true;
	}
	
	// Select
	
	void View_GameSelect::RenderGlyph(float d, int idx, bool isCenterGlyph)
	{
		const int textureIds[SELECT_COUNT] =
		{
			Textures::GAMESELECT_MODE_CLASSIC,
			Textures::GAMESELECT_MODE_BATTLE,
			Textures::GAMESELECT_MODE_SURVIVER
		};
		
		const int textureId = textureIds[idx];
		
		float dAbs = Calc::Abs(d);
		float scale = 1.0f - dAbs / SELECT_HISTORY;
		float value = Calc::Saturate(2.0f - dAbs / SELECT_HISTORY * 2.0f);
		
		if (scale <= 0.0f)
			return;
		
		scale = cosf((1.0f - scale) * Calc::mPI2);
		value = cosf((1.0f - value) * Calc::mPI2);
		
		//LOG_DBG("select render: scale: %f", scale);

		float offset = cosf((1.0f - d / SELECT_HISTORY) * Calc::mPI2) * SELECT_SIZE;
		
		Vec2F size = Vec2F(SELECT_SIZE, SELECT_SIZE) * scale;
		Vec2F position = SELECT_CENTER + Vec2F(offset, 0.0f);
		
		float c = m_SelectGlowTime > 0.0f ? (isCenterGlyph ? m_SelectGlowTime : 0.0f) : value;
		
		RenderRect(position - size / 2.0f, size, c, c, c, 1.0f, g_GameState->GetTexture(textureId));
	}
	
	void View_GameSelect::UpdateSelect(float dt)
	{
		if (m_SelectActive == false)
		{
			if (m_SelectIsAnimating)
			{
				if (m_SelectPositionD != 0.0f)
				{
					// animation stage 1: inertia
					
					m_SelectPositionD *= powf(0.1f, dt);
					
					if (Calc::Abs(m_SelectPositionD) < 0.02f)
					{
						m_SelectPositionD = 0.0f;
						m_SelectController.Setup(m_SelectPosition, Calc::RoundNearest(m_SelectPosition), SELECT_SPEED);
					}
					else
					{
						SelectPosition_set(m_SelectPosition + m_SelectPositionD);
					}
					
					UpdateSelectIndex(m_SelectPositionD);
				}
				else
				{
					// animation stage 2: lock onto final target
					
					m_SelectController.Update(dt);
					
					m_SelectPosition = m_SelectController.Angle_get();
					
					if (m_SelectPosition == m_SelectController.TargetAngle_get())
					{
						m_SelectIsAnimating = false;
					}
					
					//LOG_DBG("select update: position: %f", m_SelectPosition);
				}
			}
			else
			{
				if (m_SelectGlowTime > 0.0f)
				{
					m_SelectGlowTime -= dt;
					
					if (m_SelectGlowTime <= 0.0f)
					{
						m_DifficultyOverActive = true;
/*						switch (m_SelectedIndex)
						{
							case 0:
								g_GameState->GameBegin(GameMode_ClassicLearn, Difficulty_Easy, false);
								break;
							case 1:
								g_GameState->GameBegin(GameMode_ClassicPlay, Difficulty_Hard, false);
								break;
							case 2:
								g_GameState->GameBegin(GameMode_InvadersPlay, Difficulty_Hard, false);
								//g_GameState->ActiveView_set(::View_CustomSettings);
								break;
						}*/
					}
				}
			}
		}
		else
		{
			//m_SelectIndex = SelectIndex_get();
			
			//LOG_DBG("select update: index: %d", m_SelectIndex);
		}
	}
	
	void View_GameSelect::UpdateSelectIndex(float movementDirection)
	{	
		int idx = SelectIndex_get();
		
		if (idx != m_SelectIndex)
		{
			int increment = movementDirection < 0.0f ? -1 : +1;
			
			while (m_SelectIndex != idx)
			{
				g_GameState->m_SoundEffectMgr->Play(g_GameState->GetSound(Resources::SOUND_ZOOM_OUT), SfxFlag_MustFinish);
			
				m_SelectIndex = m_SelectIndex + increment;
				
				while (m_SelectIndex < 0)
					m_SelectIndex += SELECT_COUNT;
				while (m_SelectIndex >= SELECT_COUNT)
					m_SelectIndex -= SELECT_COUNT;
			}
		}
	}
	
	void View_GameSelect::RenderSelect()
	{
		float d = m_SelectPosition - Calc::RoundNearest(m_SelectPosition);

		// render selected + its neighbors
			
		for (int i = SELECT_HISTORY; i != 0; --i)
		{
			RenderGlyph(d + i, (m_SelectIndex - i + SELECT_COUNT) % SELECT_COUNT, false);
			RenderGlyph(d - i, (m_SelectIndex + i               ) % SELECT_COUNT, false);
		}
		
		RenderGlyph(d, m_SelectIndex, true);
	}
	
	void View_GameSelect::SelectMoveBegin()
	{
		Assert(m_SelectActive == false);
		
		m_SelectActive = true;
		
		// reset stuff
		m_SelectPositionD = 0.0f;
		m_SelectHasMoved = false;
		m_SelectGlowTime = 0.0f;
		//m_SelectIsAnimating = false;
	}
	
	void View_GameSelect::SelectMoveEnd(Vec2F position)
	{
		Assert(m_SelectActive == true);
		
		m_SelectActive = false;
		
		if (m_SelectHasMoved == false && m_SelectIsAnimating == false)
		{
			Vec2F size(SELECT_SIZE, SELECT_SIZE);
			RectF r(SELECT_CENTER - size/2.0f, size);
			if (r.IsInside(position))
			{
				m_SelectedIndex = SelectIndex_get();
				m_SelectGlowTime = SELECT_GLOW_TIME;
				g_GameState->m_SoundEffects->Play(Resources::SOUND_ZOOM_OUT, SfxFlag_MustFinish);
			}
		}
		else if (m_SelectHasMoved == false || m_SelectPositionD == 0.0f)
		{
			m_SelectPositionD = 0.0f;
			m_SelectIsAnimating = true;
			m_SelectIndex = SelectIndex_get();
			m_SelectController.Setup(m_SelectPosition, Calc::RoundNearest(m_SelectPosition), SELECT_SPEED);
		}
		else if (m_SelectHasMoved)
		{
			//m_SelectPositionD = m_SelectPositionD
			m_SelectIsAnimating = true;
		}
	}
	
	void View_GameSelect::SelectMoveUpdate(float d)
	{
		m_SelectPositionD = d;
		m_SelectHasMoved = true;
		
		SelectPosition_set(m_SelectPosition + d);
		UpdateSelectIndex(d);
	}
	
	void View_GameSelect::SelectPosition_set(float v)
	{
		const float MAX = SELECT_COUNT;
		
		while (v < 0.0f)
			v += MAX;
		while (v >= MAX)
			v -= MAX;
		
		m_SelectPosition = v;
	}
	
	int View_GameSelect::SelectIndex_get() const
	{
		int idx = (int)Calc::RoundNearest(m_SelectPosition);
		
		while (idx < 0)
			idx += SELECT_COUNT;
		while (idx >= SELECT_COUNT)
			idx -= SELECT_COUNT;
		
		return idx;
	}
#endif
}
