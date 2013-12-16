#include "EventHandler_PlayerController_Keyboard.h"
#include "EventManager.h"
#include "GameHelp.h"
#include "GameSettings.h"
#include "GameState.h"
#include "Log.h"
#include "PlayerController_Keyboard.h"
#include "World.h"

namespace Game
{
	PlayerController_Keyboard::PlayerController_Keyboard()
	{
	}

	PlayerController_Keyboard::~PlayerController_Keyboard()
	{
	}
	
	void PlayerController_Keyboard::Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, float dt)
	{
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitMove);
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitFire);
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitSpecial);

		mAutoAimController.Update(playerPosition, dt);
		mFireDir = mAutoAimController.Aim_get();
		mMoveDir = EventHandler_PlayerController_Keyboard::I().MoveDir_get();

		PlayerController::Update(playerPosition, playerPositionCorrection, dt);
	}
	
	bool PlayerController_Keyboard::IsIdle_get()
	{
		return false;
	}

	bool PlayerController_Keyboard::TouchBegin(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Keyboard::TouchMove(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Keyboard::TouchEnd(const TouchInfo& touchInfo)
	{
		return false;
	}
	
	bool PlayerController_Keyboard::TouchBegin_Fire(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Keyboard::TouchMove_Fire(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Keyboard::TouchEnd_Fire(const TouchInfo& touchInfo)
	{
		return false;
	}
	
	// Firing
	bool PlayerController_Keyboard::FireActive_get() const
	{
		if (!EventManager::I().IsActive(EVENT_PRIO_KEYBOARD))
			return false;

		return mAutoAimController.HasTarget_get();
		//return mFireDir.Length_get() >= 0.3f;
	}

	// Targeting
	float PlayerController_Keyboard::TargetingAngle_get() const
	{
		return Vec2F::ToAngle(mAutoAimController.Aim_get());
	}
	
	float PlayerController_Keyboard::TargetingSpreadScale_get() const
	{
		return 0.5f;
	}
	
	// Movement
	bool PlayerController_Keyboard::MovementActive_get() const
	{
		if (!EventManager::I().IsActive(EVENT_PRIO_KEYBOARD))
			return false;

		return 
			EventHandler_PlayerController_Keyboard::I().mKeyboardX ||
			EventHandler_PlayerController_Keyboard::I().mKeyboardY;
	}

	Vec2F PlayerController_Keyboard::MovementDirection_get() const
	{
		return mMoveDir.Normal();
	}

	float PlayerController_Keyboard::MovementSpeed_get() const
	{
		if (!MovementActive_get())
			return 0.0f;

		float length = mMoveDir.Length_get();

		return Calc::Mid(length, 0.0f, 1.0f) * PLAYER_SPEED;
	}
	
	// Special attack
	bool PlayerController_Keyboard::SpecialActive_get() const
	{
#ifndef DEPLOYMENT
		throw ExceptionNA();
#endif
		//return EventHandler_PlayerController_Keyboard::I().mKeyboardSpecialActive;
		return false;
	}
	
	// Tap action
	bool PlayerController_Keyboard::TapActive_get() const
	{
		return EventHandler_PlayerController_Keyboard::I().mKeyboardShockActive;
	}

	void PlayerController_Keyboard::TapConsume()
	{
		EventHandler_PlayerController_Keyboard::I().mKeyboardShockActive = false;
	}
	
	// Drawing
	void PlayerController_Keyboard::Render()
	{
		// nop
	}
	
	// Spawning
	void PlayerController_Keyboard::HandleSpawn(const Vec2F& playerPosition)
	{
		PlayerController::HandleSpawn(playerPosition);
	}
};
