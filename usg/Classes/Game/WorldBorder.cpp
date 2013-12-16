#include "Atlas_ImageInfo.h"
#include "GameSettings.h"
#include "GameState.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "WorldBorder.h"

namespace Game
{
	static Sprite m_Sprite;
	static float s = 7.0f;

	static void GetMinMax(int idx, Vec2F& min, Vec2F& max);

	WorldBorder::WorldBorder()
	{
	}
	
	void WorldBorder::Initialize()
	{
		m_ColorEffectTimer.Initialize(g_GameState->m_TimeTracker_World, false);
		m_OpenEffectTimer.Initialize(g_GameState->m_TimeTracker_World, false);
		
		m_ColorEffectTimer.Start(AnimTimerMode_TimeBased, false, 0.5f, AnimTimerRepeat_Mirror);
		
		//
		
		color_Opened1 = SpriteColor_Make(63, 127, 0, 255);
		color_Opened2 = SpriteColor_Make(191, 255, 63, 255);
		color_Closed1 = SpriteColor_Make(255, 0, 0, 255);
		color_Closed2 = SpriteColor_Make(255, 127, 0, 255);
		
		// create sprite
		
		const SpriteColor rgba = SpriteColors::White;
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::COLOR_BLACK);
		const float u = image->m_Info->m_TexCoord[0];
		const float v = image->m_Info->m_TexCoord[1];
		
		int vindex = 0;
		int vbase;
		int iindex = 0;
		
		m_Sprite.Allocate(16, 24);
		vbase = vindex;
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(0.0f, 0.0f, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(WORLD_SX, 0.0f, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(WORLD_SX, s, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(0.0f, s, rgba, u, v);
		m_Sprite.m_Indices[iindex++] = vbase + 0;
		m_Sprite.m_Indices[iindex++] = vbase + 1;
		m_Sprite.m_Indices[iindex++] = vbase + 2;
		m_Sprite.m_Indices[iindex++] = vbase + 0;
		m_Sprite.m_Indices[iindex++] = vbase + 2;
		m_Sprite.m_Indices[iindex++] = vbase + 3;
		
		vbase = vindex;
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(0.0f, 0.0f, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(s, 0.0f, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(s, WORLD_SY, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(0.0f, WORLD_SY, rgba, u, v);
		m_Sprite.m_Indices[iindex++] = vbase + 0;
		m_Sprite.m_Indices[iindex++] = vbase + 1;
		m_Sprite.m_Indices[iindex++] = vbase + 2;
		m_Sprite.m_Indices[iindex++] = vbase + 0;
		m_Sprite.m_Indices[iindex++] = vbase + 2;
		m_Sprite.m_Indices[iindex++] = vbase + 3;
		
		vbase = vindex;
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(WORLD_SX - s, 0.0f, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(WORLD_SX, 0.0f, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(WORLD_SX, WORLD_SY, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(WORLD_SX - s, WORLD_SY, rgba, u, v);
		m_Sprite.m_Indices[iindex++] = vbase + 0;
		m_Sprite.m_Indices[iindex++] = vbase + 1;
		m_Sprite.m_Indices[iindex++] = vbase + 2;
		m_Sprite.m_Indices[iindex++] = vbase + 0;
		m_Sprite.m_Indices[iindex++] = vbase + 2;
		m_Sprite.m_Indices[iindex++] = vbase + 3;
		
		vbase = vindex;
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(0.0f, WORLD_SY - s, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(WORLD_SX, WORLD_SY - s, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(WORLD_SX, WORLD_SY, rgba, u, v);
		m_Sprite.m_Vertices[vindex++] = SpriteVertex(0.0f, WORLD_SY, rgba, u, v);
		m_Sprite.m_Indices[iindex++] = vbase + 0;
		m_Sprite.m_Indices[iindex++] = vbase + 1;
		m_Sprite.m_Indices[iindex++] = vbase + 2;
		m_Sprite.m_Indices[iindex++] = vbase + 0;
		m_Sprite.m_Indices[iindex++] = vbase + 2;
		m_Sprite.m_Indices[iindex++] = vbase + 3;
	}
	
	void WorldBorder::Update(float dt)
	{
		if (m_OpenTrigger.Read())
			m_OpenTrigger.Stop();
	}
	
	void WorldBorder::Render()
	{
		// calculate color
		
		SpriteColor color;
		
		if (IsClosed_get())
			color = SpriteColor_BlendF(color_Closed1, color_Closed2, m_ColorEffectTimer.Progress_get());
		else
			color = SpriteColor_BlendF(color_Opened1, color_Opened2, m_ColorEffectTimer.Progress_get());
		
		if (m_OpenEffectTimer.IsRunning_get())
			color = SpriteColor_BlendF(color, SpriteColors::White, m_OpenEffectTimer.Progress_get());
		
#if defined(PSP) || 1
		//g_GameState->DataSetActivate(DS_GAME);

		const AtlasImageMap* image = g_GameState->GetTexture(Textures::COLOR_BLACK);

		for (int i = 0; i < 4; ++i)
		{
			Vec2F min, max;
			GetMinMax(i, min, max);
			RenderRect(min, max - min, color, image);
		}
#else
		// update sprite color
		
		for (int i = 0; i < m_Sprite.m_VertexCount; ++i)
			m_Sprite.m_Vertices[i].m_Color = color;
		
		// draw sprite
		
		g_GameState->DataSetActivate(DS_GAME);

		g_GameState->m_SpriteGfx->WriteSprite(m_Sprite);
#endif
	}
	
	void WorldBorder::Close(float duration)
	{
		m_OpenTrigger.Start(duration);
	}
	
	bool WorldBorder::IsClosed_get() const
	{
		return m_OpenTrigger.IsRunning_get() != XFALSE;
	}

	static void GetMinMax(int idx, Vec2F& min, Vec2F& max)
	{
		switch (idx)
		{
		case 0:
			min.Set(0.0f, 0.0f);
			max.Set(WORLD_SX, s);
			break;
		
		case 1:
			min.Set(0.0f, 0.0f);
			max.Set(s, WORLD_SY);
			break;
		
		case 2:
			min.Set(WORLD_SX - s, 0.0f);
			max.Set(WORLD_SX, WORLD_SY);
			break;
		
		case 3:
			min.Set(0.0f, WORLD_SY - s);
			max.Set(WORLD_SX, WORLD_SY);
			break;

		default:
			Assert(false);
			min.SetZero();
			max.SetZero();
			break;
		}
	}
}
