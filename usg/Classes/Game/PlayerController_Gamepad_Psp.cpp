#include "EventHandler_PlayerController_Gamepad_Psp.h"
#include "EventManager.h"
#include "GameHelp.h"
#include "GameRound.h"
#include "GameSettings.h"
#include "GameState.h"
#include "Log.h"
#include "PlayerController_Gamepad_Psp.h"
#include "World.h"

namespace Game
{
	PlayerController_Gamepad_Psp::PlayerController_Gamepad_Psp()
	{
	}

	PlayerController_Gamepad_Psp::~PlayerController_Gamepad_Psp()
	{
	}
	
	void PlayerController_Gamepad_Psp::Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, float dt)
	{
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitMove);
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitFire);
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitSpecial);
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitUpgrade);
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitWeaponSwitch);

		mFireDir = EventHandler_PlayerController_Gamepad_Psp::I().mJoyAxis_Fire;
		mMoveDir = EventHandler_PlayerController_Gamepad_Psp::I().mJoyAxis_Move;

		PlayerController::Update(playerPosition, playerPositionCorrection, dt);

		UpdateAutoAim(playerPosition, dt);
	}

	bool PlayerController_Gamepad_Psp::IsIdle_get()
	{
		return false;
	}

	bool PlayerController_Gamepad_Psp::TouchBegin(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Gamepad_Psp::TouchMove(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Gamepad_Psp::TouchEnd(const TouchInfo& touchInfo)
	{
		return false;
	}
	
	bool PlayerController_Gamepad_Psp::TouchBegin_Fire(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Gamepad_Psp::TouchMove_Fire(const TouchInfo& touchInfo)
	{
		return false;
	}

	bool PlayerController_Gamepad_Psp::TouchEnd_Fire(const TouchInfo& touchInfo)
	{
		return false;
	}
	
	// Firing
	bool PlayerController_Gamepad_Psp::FireActive_get() const
	{
		if (!EventManager::I().IsActive(EVENT_PRIO_JOYSTICK))
			return false;

		if (g_GameState->m_GameRound->GameMode_get() == GameMode_IntroScreen || g_World->m_TouchZoomController.IsActive(ZoomTarget_ZoomedOut))
			return false;

		return mAutoAimController.HasTarget_get();
	}
	
	// Targeting
	float PlayerController_Gamepad_Psp::TargetingAngle_get() const
	{
		return Vec2F::ToAngle(mFireDir);
	}

	float PlayerController_Gamepad_Psp::TargetingSpreadScale_get() const
	{
		return 0.5f;
	}

	float PlayerController_Gamepad_Psp::DrawingAngle_get() const
	{
		return PlayerController::DrawingAngle_get();
	}
	
	// Movement
	bool PlayerController_Gamepad_Psp::MovementActive_get() const
	{
		if (!EventManager::I().IsActive(EVENT_PRIO_JOYSTICK))
			return false;

		return EventHandler_PlayerController_Gamepad_Psp::I().mJoyAxis_Move.Length_get() >= 0.2f;
	}

	Vec2F PlayerController_Gamepad_Psp::MovementDirection_get() const
	{
		return mMoveDir.Normal();
	}

	float PlayerController_Gamepad_Psp::MovementSpeed_get() const
	{
		float length = mMoveDir.Length_get();

		return Calc::Mid(length, 0.0f, 1.0f) * PLAYER_SPEED;
	}
	
	// Special attack
	bool PlayerController_Gamepad_Psp::SpecialActive_get() const
	{
		return EventHandler_PlayerController_Gamepad_Psp::I().mJoySpecialActive;
	}
	
	// Tap action
	bool PlayerController_Gamepad_Psp::TapActive_get() const
	{
		return EventHandler_PlayerController_Gamepad_Psp::I().mJoyShockActive;
	}

	void PlayerController_Gamepad_Psp::TapConsume()
	{
		EventHandler_PlayerController_Gamepad_Psp::I().mJoyShockActive = false;
	}
	
	// Drawing
	void PlayerController_Gamepad_Psp::Render()
	{
		// nop
	}
	
	// Spawning
	void PlayerController_Gamepad_Psp::HandleSpawn(const Vec2F& playerPosition)
	{
		PlayerController::HandleSpawn(playerPosition);
	}

	// Auto aim
	void PlayerController_Gamepad_Psp::UpdateAutoAim(const Vec2F& playerPosition, float dt)
	{
		mAutoAimController.Update(playerPosition, dt);

		mFireDir = mAutoAimController.Aim_get();
	}
};
