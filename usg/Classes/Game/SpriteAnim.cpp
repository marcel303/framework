#include "GameState.h"
#include "SpriteAnim.h"
#include "TempRender.h"

namespace Game
{
	SpriteAnim::SpriteAnim()
	{
		mTexture = 0;
		mActiveFrame = 0;
		mActiveLoop = 0;
	}
	
	void SpriteAnim::Setup(Res* texture, int gridSx, int gridSy, int frameBegin, int frameEnd, int fps, ITimer* timer)
	{
		mTexture = texture;
		mGridSx = gridSx;
		mGridSy = gridSy;
		mFrameBegin = frameBegin;
		mFrameEnd = frameEnd;
		mFps = fps;
		mTimer.Initialize(timer);
	}
	
	void SpriteAnim::Start()
	{
		mTimer.Start();
	}
	
	void SpriteAnim::Update()
	{
		float delta = mTimer.Delta_get();
		
		int frame = (int)(delta * mFps);
		int frameCount = FrameCount_get();
		
		mActiveFrame = mFrameBegin + frame % frameCount;	
		mActiveLoop = frame / frameCount;
	}
	
	void SpriteAnim::Render(Vec2F position, Vec2F size, SpriteColor color)
	{
		SpriteGfx& gfx = *g_GameState->m_SpriteGfx;
		
		int frame = mActiveFrame;
		
		int cellX = frame % mGridSx;
		int cellY = frame / mGridSy;
		
		float u = cellX / (float)mGridSx;
		float v = cellY / (float)mGridSy;
		float su = 1.0f / mGridSx;
		float sv = 1.0f / mGridSy;
		
		DrawRect(gfx, position[0], position[1], size[0], size[1], u, v, su, sv, color.rgba);
	}
}
