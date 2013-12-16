#pragma once

#include "AnimTimer.h"
#include "DeltaTimer.h"
#include "Forward.h"

namespace Game
{
	class SpriteAnim
	{
	public:
		SpriteAnim();
		void Setup(Res* texture, int gridSx, int gridSy, int frameBegin, int frameEnd, int fps, ITimer* timer);
		
		void Start();
		void Update();
		void Render(Vec2F position, Vec2F size, SpriteColor color);
		inline Res* Texture_get() const
		{
			return mTexture;
		}
		inline int ActiveLoop_get() const
		{
			return mActiveLoop;
		}
		
	private:
		int Frame_get() const;
		inline int FrameCount_get() const
		{
			return mFrameEnd - mFrameBegin + 1;
		}
		
		Res* mTexture;
		int mGridSx;
		int mGridSy;
		int mFrameBegin;
		int mFrameEnd;
		int mFps;
		int mActiveFrame;
		int mActiveLoop;
		DeltaTimer mTimer;
	};
}
