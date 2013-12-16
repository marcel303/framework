#pragma once

#include "EventHandler.h"
#include "EventHandler.h"
#include "PlayerController.h"
#include "PlayerController_AutoAim.h"

namespace Game
{
	class PlayerController_Keyboard : public PlayerController_DualAnalog
	{
	public:
		PlayerController_Keyboard();
		~PlayerController_Keyboard();

		virtual void Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, float dt);
		virtual bool IsIdle_get();
		
		virtual bool TouchBegin(const TouchInfo& touchInfo);
		virtual bool TouchMove(const TouchInfo& touchInfo);
		virtual bool TouchEnd(const TouchInfo& touchInfo);
		virtual bool TouchBegin_Fire(const TouchInfo& touchInfo);
		virtual bool TouchMove_Fire(const TouchInfo& touchInfo);
		virtual bool TouchEnd_Fire(const TouchInfo& touchInfo);
		
		// Firing
		virtual bool FireActive_get() const;
		
		// Targeting
		virtual float TargetingAngle_get() const;
		virtual float TargetingSpreadScale_get() const;
		//virtual float DrawingAngle_get() const;
		
		// Movement
		virtual bool MovementActive_get() const;
		virtual Vec2F MovementDirection_get() const;
		virtual float MovementSpeed_get() const;
		
		// Special attack
		virtual bool SpecialActive_get() const;
		
		// Tap action
		virtual bool TapActive_get() const;
		virtual void TapConsume();
		
		// Drawing
		virtual void Render();
		
		// Spawning
		virtual void HandleSpawn(const Vec2F& playerPosition);

	private:
		Vec2F mMoveDir;
		Vec2F mFireDir;
		AutoAimController mAutoAimController;
	};
}
