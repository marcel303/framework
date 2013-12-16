#pragma once

#include "SpriteAnim.h"
#include "SpriteGfx.h"

#define MAX_SPRITE_ANIM 100

namespace Game
{
	class SpriteEffect
	{
	public:
		SpriteEffect()
		{
			mIsAllocated = false;
		}
		
		SpriteEffect(const SpriteAnim& anim, const Vec2F& position, const Vec2F& size, SpriteColor color)
		{
			mAnim = anim;
			mPosition = position;
			mSize = size;
			mColor = color;
			mIsAllocated = true;
		}
		
		void Update()
		{
			mAnim.Update();
			
			if (mAnim.ActiveLoop_get() > 0)
				mIsAllocated = false;
		}
		
		void Render()
		{
			mAnim.Render(mPosition, mSize, mColor);
		}
		
		SpriteAnim mAnim;
		Vec2F mPosition;
		Vec2F mSize;
		SpriteColor mColor;
		bool mIsAllocated;
	};
	
	class SpriteEffectMgr
	{
	public:
		void Add(const SpriteAnim& anim, const Vec2F& position, const Vec2F& size, SpriteColor color);
		void Update();
		void Render();
		
	private:
		void SortByTexture();
		void Remove(int index);
		
		SpriteEffect mAnimList[MAX_SPRITE_ANIM];
	};
}
