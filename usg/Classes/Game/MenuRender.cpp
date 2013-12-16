#include "GameState.h"
#include "GuiButton.h"
#include "Menu.h"
#include "MenuRender.h"
#include "TempRender.h"
#include "TextureAtlas.h"
#include "Textures.h"
#include "UsgResources.h"

namespace GameMenu
{
	void Render_Empty(void* obj, void* arg)
	{
		// nop
	}
	
	void HitEffect_Particles_Centroid(const Vec2F& pos)
	{
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::ENEMY_EXPLOSION);
		
		for (int i = 0; i < 150; ++i)
		{
			Particle& p = g_GameState->m_ParticleEffect_UI.Allocate(image->m_Info, 0, 0);
			
			Particle_Default_Setup(
				&p,
				pos[0] + Calc::Random_Scaled(10.0f),
				pos[1] + Calc::Random_Scaled(10.0f), 1.0f, 10.0f, 4.0f, Calc::Random(Calc::mPI * 2.0f), 150.0f);
		}
	}
	
	void HitEffect_Particles_Rect(const RectF& rect)
	{
		const AtlasImageMap* image = g_GameState->GetTexture(Textures::PARTICLE_MENU);
		
		Vec2F basePos = rect.m_Position + rect.m_Size * 0.5f;
		
		int count = 80;
		float angle = 0.0f;
		float angleStep = Calc::m2PI / count;
		
		float life[3] = { 1.0f, 0.8f, 0.6f };
		SpriteColor color[3] = { SpriteColor_Make(255, 255, 255, 255), SpriteColor_Make(255, 0, 255, 255), SpriteColor_Make(0, 0, 255, 255) };
		float speed[3] = { 150.0f, 130.0f, 110.0f };
		
		for (int j = 0; j < 3; ++j)
		{
			for (int i = 0; i < count; ++i)
			{
				Particle& p = g_GameState->m_ParticleEffect_UI.Allocate(image->m_Info, 0, 0);
				
				Vec2F dir = Vec2F::FromAngle(angle);
				
				float t = 0.0f;
				
				Intersect_Rect(rect, basePos, dir, t);
				
				Vec2F pos = basePos + dir * t;
				
				Particle_Default_Setup(
					&p,
					pos[0],
					pos[1], life[j] * 0.5f, 10.0f, 4.0f, angle, speed[j]);
				
				p.m_Color = color[j];
				
				angle += angleStep;
			}
		}
	}
}
