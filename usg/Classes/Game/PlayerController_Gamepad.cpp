#include "EventHandler_PlayerController_Gamepad.h"
#include "EventManager.h"
#include "GameHelp.h"
#include "GameSettings.h"
#include "GameState.h"
#include "Log.h"
#include "PlayerController_Gamepad.h"
#include "World.h"

namespace Game
{
	PlayerController_Gamepad::PlayerController_Gamepad()
	{
	}

	PlayerController_Gamepad::~PlayerController_Gamepad()
	{
	}
	
	void PlayerController_Gamepad::Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, float dt)
	{
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitMove);
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitFire);
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitSpecial);
#if defined(BBOS)
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitUpgrade);
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitWeaponSwitch);
#endif

		mFireDir = EventHandler_PlayerController_Gamepad::I().mJoyAxis_Fire;
		mMoveDir = EventHandler_PlayerController_Gamepad::I().mJoyAxis_Move;

		PlayerController::Update(playerPosition, playerPositionCorrection, dt);
	}

	bool PlayerController_Gamepad::IsIdle_get()
	{
		return false;
	}
		
	bool PlayerController_Gamepad::TouchBegin(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Gamepad::TouchMove(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Gamepad::TouchEnd(const TouchInfo& touchInfo)
	{
		return false;
	}
	
	bool PlayerController_Gamepad::TouchBegin_Fire(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Gamepad::TouchMove_Fire(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Gamepad::TouchEnd_Fire(const TouchInfo& touchInfo)
	{
		return false;
	}
	
	// Firing
	bool PlayerController_Gamepad::FireActive_get() const
	{
		if (!EventManager::I().IsActive(EVENT_PRIO_JOYSTICK))
			return false;

		return mFireDir.Length_get() >= 0.3f;
	}
	
	// Targeting
	float PlayerController_Gamepad::TargetingAngle_get() const
	{
		return Vec2F::ToAngle(mFireDir);
	}

	float PlayerController_Gamepad::TargetingSpreadScale_get() const
	{
		return 0.5f;
	}

	float PlayerController_Gamepad::DrawingAngle_get() const
	{
		return PlayerController::DrawingAngle_get();
	}
	
	// Movement
	bool PlayerController_Gamepad::MovementActive_get() const
	{
		if (!EventManager::I().IsActive(EVENT_PRIO_JOYSTICK))
			return false;

		return EventHandler_PlayerController_Gamepad::I().mJoyAxis_Move.Length_get() >= 0.1f;
	}

	Vec2F PlayerController_Gamepad::MovementDirection_get() const
	{
		return mMoveDir.Normal();
	}

	float PlayerController_Gamepad::MovementSpeed_get() const
	{
		if (MovementActive_get() == false)
			return 0.0f;

		float length = mMoveDir.Length_get();

		return Calc::Mid(length, 0.0f, 1.0f) * PLAYER_SPEED;
	}
	
	// Special attack
	bool PlayerController_Gamepad::SpecialActive_get() const
	{
		return EventHandler_PlayerController_Gamepad::I().mJoySpecialActive;
	}
	
	// Tap action
	bool PlayerController_Gamepad::TapActive_get() const
	{
		return EventHandler_PlayerController_Gamepad::I().mJoyShockActive;
	}

	void PlayerController_Gamepad::TapConsume()
	{
		EventHandler_PlayerController_Gamepad::I().mJoyShockActive = false;
	}
	
	// Drawing
	void PlayerController_Gamepad::Render()
	{
		// nop

		//PlayerController::Render();
	}
	
	// Spawning
	void PlayerController_Gamepad::HandleSpawn(const Vec2F& playerPosition)
	{
		PlayerController::HandleSpawn(playerPosition);
	}
};
