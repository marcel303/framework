#include "GameState.h"
#include "Graphics.h"
#include "Log.h"
#include "SpriteEffectMgr.h"

namespace Game
{
	void SpriteEffectMgr::Add(const SpriteAnim& anim, const Vec2F& position, const Vec2F& size, SpriteColor color)
	{
		for (int i = 0; i < MAX_SPRITE_ANIM; ++i)
		{
			if (mAnimList[i].mIsAllocated)
				continue;
			
			mAnimList[i] = SpriteEffect(anim, position, size, color);
			
//			LOG(LogLevel_Debug, "allocated sprite. index: %d", i);
			
			return;
		}
		
//		LOG(LogLevel_Debug, "sprite mgr full");
	}
	
	void SpriteEffectMgr::Update()
	{
		for (int i = 0; i < MAX_SPRITE_ANIM; ++i)
		{
			if (!mAnimList[i].mIsAllocated)
				continue;
			
			mAnimList[i].Update();
		}
	}
	
	void SpriteEffectMgr::Render()
	{
		SpriteGfx& gfx = *g_GameState->m_SpriteGfx;
		gfx.Flush();
		
		SortByTexture();
		
		Res* texture = 0;
		
		for (int i = 0; i < MAX_SPRITE_ANIM; ++i)
		{
			if (!mAnimList[i].mIsAllocated)
				continue;
			
			Res* texture2 = mAnimList[i].mAnim.Texture_get();
			
			if (texture2 != texture)
			{
//				LOG(LogLevel_Debug, "sprite texture swap");
				
				gfx.Flush();
				gGraphics.TextureSet(texture2);
				texture = texture2;
			}
			
			mAnimList[i].Render();
		}
		
		gfx.Flush();
	}
	
	static int SortCB(const void* e1, const void* e2)
	{
		const SpriteEffect* s1 = (const SpriteEffect*)e1;
		const SpriteEffect* s2 = (const SpriteEffect*)e2;
		
		if (s1->mAnim.Texture_get() < s2->mAnim.Texture_get())
			return -1;
		if (s1->mAnim.Texture_get() > s2->mAnim.Texture_get())
			return +1;

		return 0;
	}
	
	void SpriteEffectMgr::SortByTexture()
	{
		qsort(mAnimList, MAX_SPRITE_ANIM, sizeof(SpriteEffect), SortCB);
	}
}
