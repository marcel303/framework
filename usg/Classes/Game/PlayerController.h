#pragma once

#include "AnalogController.h"
#include "AngleController.h"
#include "IPlayerController.h"
#include "PlayerController_AutoAim.h"
#include "Selection.h"
#include "SpecialController.h"
#include "TargetingController.h"
#include "TriggerTimerEx.h"
#include "Types.h"
#include "Util_Follower.h"

namespace Game
{	
	// Player input controller
	//
	// Takes touh begin/end/move events as input and decides in which direction the ship should move, etc
	
	class PlayerController: public IPlayerController
	{
	public:
		PlayerController();
		virtual ~PlayerController();
		void Initialize();
		void Shutdown();
		
		virtual void Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, float dt);
		virtual bool IsIdle_get();
		
		virtual bool TouchBegin(const TouchInfo& touchInfo);
		virtual bool TouchMove(const TouchInfo& touchInfo);
		virtual bool TouchEnd(const TouchInfo& touchInfo);
		
		virtual bool TouchBegin_Fire(const TouchInfo& touchInfo);
		virtual bool TouchMove_Fire(const TouchInfo& touchInfo);
		virtual bool TouchEnd_Fire(const TouchInfo& touchInfo);

		virtual bool FireActive_get() const;
		virtual float TargetingAngle_get() const;
		virtual float TargetingSpreadScale_get() const;
		virtual float DrawingAngle_get() const;
		virtual bool MovementActive_get() const;
		virtual Vec2F MovementDirection_get() const;
		virtual float MovementSpeed_get() const;
//		virtual bool SpecialActive_get() const;
		virtual bool TapActive_get() const;
		virtual void TapConsume();
		virtual void TiltCalibrate();
		
		virtual void HandleSpawn(const Vec2F& playerPosition);
		
		virtual void Render();
		
	protected:
		void RenderShockwaveButton(bool showLevel0);
		
		AnalogController m_MovementController;
		AnalogController m_FiringController; // only used to detect touch / no touch finger state
		TargetingController m_TargetingController;
		AngleController m_MovementDirectionController;
//		SpecialController m_SpecialController;
//		RoundButtonController m_SpecialController;
		RoundButtonController m_ShockwaveController;
		bool m_TapActive;
	};
	
	class PlayerController_DualAnalog : public PlayerController
	{
	private:
		virtual bool IsIdle_get();

		virtual float TargetingAngle_get() const;
		virtual float TargetingSpreadScale_get() const;

		virtual void HandleSpawn(const Vec2F& playerPosition);
	};
	
	class PlayerController_Tilt : public PlayerController
	{
	public:
		PlayerController_Tilt();
		
		virtual void Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, float dt);
		virtual bool IsIdle_get();

		virtual bool FireActive_get() const;
		virtual float TargetingAngle_get() const;
		virtual float TargetingSpreadScale_get() const;
//		virtual float DrawingAngle_get() const;
		virtual bool MovementActive_get() const;
		virtual Vec2F MovementDirection_get() const;
		virtual float MovementSpeed_get() const;
		virtual void TiltCalibrate();
		
		virtual void Render();
		
	protected:
		//CD_TYPE SelectAutoAimTarget();
		void UpdateAutoAim(float dt);
		
		//TriggerTimerW mUpdateTrigger;
		//CD_TYPE mTarget;
		Vec2F mDirection;
		Vec2F mPosition;
		//AngleController mAngleController;
		Vec2F mTilt;
		float mCalibrationAngle;
		bool mCalibrationUpsideDown;
		AutoAimController mAutoAimController;
	};
}
