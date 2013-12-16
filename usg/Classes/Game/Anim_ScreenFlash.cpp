#include "Anim_ScreenFlash.h"
#include "GameSettings.h"
#include "GameState.h"
#include "TempRender.h"
#include "Textures.h"

namespace Game
{
	Anim_ScreenFlash::Anim_ScreenFlash()
	{
		Initialize();
	}
	
	void Anim_ScreenFlash::Initialize()
	{
		mFlashAnim.Initialize(g_GameState->m_TimeTracker_World, false);
	}
	
	void Anim_ScreenFlash::Start(int frameCount, SpriteColor color)
	{
		mFlashAnim.Start(AnimTimerMode_FrameBased, false, (float)frameCount, AnimTimerRepeat_None);
		mColor = color;
	}
	
	void Anim_ScreenFlash::Render()
	{
		if (mFlashAnim.IsRunning_get())
		{
			const AtlasImageMap* image = g_GameState->GetTexture(Textures::COLOR_BLACK);
			// todo: use DrawRect method
			//float u = image->m_Info->m_TexCoord[0];
			//float v = image->m_Info->m_TexCoord[1];
			float t = 1.0f - mFlashAnim.Progress_get();
			int c = (int)(t * 255.0f);
			mColor.v[3] = c;
			RenderRect(Vec2F(0.0f, 0.0f), Vec2F(WORLD_SX, WORLD_SY), mColor.v[0] / 255.0f, mColor.v[1] / 255.0f, mColor.v[2] / 255.0f, mColor.v[3] / 255.0f, image);
			/*SpriteGfx& gfx = g_GameState->m_SpriteGfx;
			gfx.Reserve(4, 6);
			gfx.WriteBegin();
			gfx.WriteVertex(0.0f, 0.0f, mColor.rgba, u, v);
			gfx.WriteVertex(WORLD_SX, 0.0f, mColor.rgba, u, v);
			gfx.WriteVertex(WORLD_SX, WORLD_SY, mColor.rgba, u, v);
			gfx.WriteVertex(0.0f, WORLD_SY, mColor.rgba, u, v);
			gfx.WriteIndex(0);
			gfx.WriteIndex(1);
			gfx.WriteIndex(2);
			gfx.WriteIndex(0);
			gfx.WriteIndex(2);
			gfx.WriteIndex(3);
			gfx.WriteEnd();*/
		}
		
		mFlashAnim.Tick();
	}
}
