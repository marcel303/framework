#pragma once

#include "AnimTimer.h"
#include "PolledTimer.h"
#include "SpriteGfx.h"
#include "TriggerTimerEx.h"

namespace Game
{
	class WorldBorder
	{
	public:
		WorldBorder();
		void Initialize();
		
		void Update(float dt);
		void Render();
		
		void Close(float duration);
		
		bool IsClosed_get() const;
		
		Sprite m_Sprite;
		TriggerTimerW m_OpenTrigger;
		AnimTimer m_ColorEffectTimer;
		AnimTimer m_OpenEffectTimer;
		SpriteColor color_Opened1;
		SpriteColor color_Opened2;
		SpriteColor color_Closed1;
		SpriteColor color_Closed2;
	};
}
