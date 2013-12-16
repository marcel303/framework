#pragma once

#include "AnimTimer.h"
#include "SpriteGfx.h"

namespace Game
{
	class Anim_ScreenFlash
	{
	public:
		Anim_ScreenFlash();
		void Initialize();
		
		void Start(int frameCount, SpriteColor color);
		void Render();
		
	private:
		AnimTimer mFlashAnim;
		SpriteColor mColor;
	};
}
