#pragma once

#include "TouchInfo.h"
#include "Types.h"

namespace Game
{
	class IPlayerController
	{
	public:
		virtual ~IPlayerController();
		
		virtual void Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, float dt) = 0;
		
		virtual bool IsIdle_get() = 0; // when true, the game is auto-paused

		static bool HandleTouchBegin(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchMove(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchEnd(void* obj, const TouchInfo& touchInfo);
		
		static bool HandleTouchBegin_Fire(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchMove_Fire(void* obj, const TouchInfo& touchInfo);
		static bool HandleTouchEnd_Fire(void* obj, const TouchInfo& touchInfo);
		
		virtual bool TouchBegin(const TouchInfo& touchInfo) = 0;
		virtual bool TouchMove(const TouchInfo& touchInfo) = 0;
		virtual bool TouchEnd(const TouchInfo& touchInfo) = 0;
		
		virtual bool TouchBegin_Fire(const TouchInfo& touchInfo) = 0;
		virtual bool TouchMove_Fire(const TouchInfo& touchInfo) = 0;
		virtual bool TouchEnd_Fire(const TouchInfo& touchInfo) = 0;
		
		// Firing
		virtual bool FireActive_get() const = 0;
		
		// Targeting
		virtual float TargetingAngle_get() const = 0;
		virtual float TargetingSpreadScale_get() const = 0;
		virtual float DrawingAngle_get() const = 0;
		
		// Movement
		virtual bool MovementActive_get() const = 0;
		virtual Vec2F MovementDirection_get() const = 0;
		virtual float MovementSpeed_get() const = 0;
		
		// Special attack
//		virtual bool SpecialActive_get() const = 0;
		
		// Tap action
		virtual bool TapActive_get() const = 0;
		virtual void TapConsume() = 0;
		
		// Tilt
		virtual void TiltCalibrate() = 0;
		
		// Drawing
		virtual void Render() = 0;
		
		// Spawning
		virtual void HandleSpawn(const Vec2F& playerPosition) = 0;
	};
}
