#pragma once

#include "AnimTimer.h"

namespace Game
{
	class ScreenLock
	{
	public:
		ScreenLock();
		void Initialize(const char* text, bool altMode);
		void Initialize(const char* text);
		
		void Start(bool isActive);
		float FadeTime_get() const;
		
		// --------------------
		// Animation
		// --------------------
	private:
		void Update();
	public:
		void Render();
		
		const char* m_Text;
		bool m_AltMode;
		bool m_IsActive;
		AnimTimer m_LockAnim;
		float m_LockPos[2];
		float m_BounceSpeed;
		float m_BouncePos;
	};
}
