#include "AngleController.h"
#include "Benchmark.h"
#include "EntityPlayer.h"
#include "PlayerController.h"

#include "GameHelp.h"
#include "GameRound.h"
#include "GameSettings.h"
#include "GameState.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "World.h"

#include "System.h" // Tilt movement

#define TAPTIME 0.5

#define CALIBRATION_ANGLE g_GameState->m_GameSettings->m_TiltControl_CalibrationAngle

namespace Game
{
	IPlayerController::~IPlayerController()
	{
	}
	
	bool IPlayerController::HandleTouchBegin(void* obj, const TouchInfo& touchInfo)
	{
		PlayerController* self = (PlayerController*)obj;
		
		return self->TouchBegin(touchInfo);
	}
	
	bool IPlayerController::HandleTouchMove(void* obj, const TouchInfo& touchInfo)
	{
		PlayerController* self = (PlayerController*)obj;
		
		return self->TouchMove(touchInfo);
	}
	
	bool IPlayerController::HandleTouchEnd(void* obj, const TouchInfo& touchInfo)
	{
		PlayerController* self = (PlayerController*)obj;
		
		return self->TouchEnd(touchInfo);
	}
	
	bool IPlayerController::HandleTouchBegin_Fire(void* obj, const TouchInfo& touchInfo)
	{
		PlayerController* self = (PlayerController*)obj;
		
		return self->TouchBegin_Fire(touchInfo);
	}
	
	bool IPlayerController::HandleTouchMove_Fire(void* obj, const TouchInfo& touchInfo)
	{
		PlayerController* self = (PlayerController*)obj;
		
		return self->TouchMove_Fire(touchInfo);
	}
	
	bool IPlayerController::HandleTouchEnd_Fire(void* obj, const TouchInfo& touchInfo)
	{
		PlayerController* self = (PlayerController*)obj;
		
		return self->TouchEnd_Fire(touchInfo);
	}
	
	//
	
	PlayerController::PlayerController() : IPlayerController()
	{
		Initialize();
	}
	
	PlayerController::~PlayerController()
	{
		Shutdown();
	}
	
	void PlayerController::Initialize()
	{
		float size = 50.0f;
		float border = size + 20.0f;
		
		m_MovementController.Setup(Vec2F(border, VIEW_SY - border), size, 3.0f);
		m_FiringController.Setup(Vec2F(VIEW_SX - border, VIEW_SY - border), size, 0.0f);
		m_MovementDirectionController.Setup(-Calc::mPI2, -Calc::mPI2, Calc::m4PI);
		//m_SpecialController.Setup(m_FiringController.Position_get(), Calc::DegToRad(-105.0f), Calc::DegToRad(0.0f), 50.0f, 65.0f);
		//m_SpecialController.Setup(Vec2F(VIEW_SX - 35.0f, 165.0f), 17.0f);
		m_TapActive = false;
		
		// register touch listeners
		
		TouchListener listener;
		
		listener.Setup(this, HandleTouchBegin, HandleTouchEnd, HandleTouchMove);
		g_GameState->m_TouchDLG->Register(USG::TOUCH_PRIO_CONTROLLERS, listener);
		
		listener.Setup(this, HandleTouchBegin_Fire, HandleTouchEnd_Fire, HandleTouchMove_Fire);
		g_GameState->m_TouchDLG->Register(USG::TOUCH_PRIO_CONTROLLER_FIRE, listener);
	}
	
	void PlayerController::Shutdown()
	{
		g_GameState->m_TouchDLG->Unregister(USG::TOUCH_PRIO_CONTROLLERS);
		g_GameState->m_TouchDLG->Unregister(USG::TOUCH_PRIO_CONTROLLER_FIRE);
	}

	void PlayerController::Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, float dt)
	{
		m_TargetingController.Update(playerPosition, playerPositionCorrection, m_FiringController.IsActive_get());
		
		m_MovementDirectionController.TargetAngle_set(Vec2F::ToAngle(MovementDirection_get()));
		m_MovementDirectionController.Update(dt);
	}
	
	bool PlayerController::IsIdle_get()
	{
		return
			!MovementActive_get() &&
			!FireActive_get();
	}

	//
	
	bool PlayerController::TouchBegin(const TouchInfo& touchInfo)
	{
		if (g_GameState->m_HelpState->IsCompleteOrActive(HelpState::State_HitMove))
		{
			bool calibrateMove = g_GameState->m_GameSettings->m_ControllerModeMove == ControllerMode_AutoCalibrate;
			
			if (m_MovementController.TouchBegin(touchInfo.m_FingerIndex, touchInfo.m_LocationView, calibrateMove))
			{		
				g_GameState->m_HelpState->DoComplete(HelpState::State_HitMove);
				
				return true;
			}
		}
		
		return false;
	}
	
	bool PlayerController::TouchEnd(const TouchInfo& touchInfo)
	{
		m_MovementController.TouchEnd(touchInfo.m_FingerIndex);
//		m_SpecialController.TouchEnd(touchInfo.m_FingerIndex);
		
		return true;
	}
	
	bool PlayerController::TouchMove(const TouchInfo& touchInfo)
	{
		m_MovementController.TouchMove(touchInfo.m_FingerIndex, touchInfo.m_LocationView);
//		m_SpecialController.TouchMove(touchInfo.m_FingerIndex, touchInfo.m_LocationView);
		
		return true;
	}
	
	//
	
	bool PlayerController::TouchBegin_Fire(const TouchInfo& touchInfo)
	{
		if (g_GameState->m_HelpState->IsCompleteOrActive(HelpState::State_HitFire))
		{
			bool calibrateFire = g_GameState->m_GameSettings->m_ControllerModeFire == ControllerMode_AutoCalibrate;
			
			if (m_FiringController.TouchBegin(touchInfo.m_FingerIndex, touchInfo.m_LocationView, calibrateFire))
			{
				if (touchInfo.m_TapInterval < TAPTIME)
				{
					m_TapActive = true;
				}

				g_GameState->m_HelpState->DoComplete(HelpState::State_HitFire);
				
				return true;
			}
		}
		
		return false;
	}
	
	bool PlayerController::TouchEnd_Fire(const TouchInfo& touchInfo)
	{
		m_FiringController.TouchEnd(touchInfo.m_FingerIndex);
		
		return true;
	}
	
	bool PlayerController::TouchMove_Fire(const TouchInfo& touchInfo)
	{
		m_FiringController.TouchMove(touchInfo.m_FingerIndex, touchInfo.m_LocationView);
		
		return true;
	}
	
	//
	
	bool PlayerController::FireActive_get() const
	{
		return m_FiringController.IsActive_get();
	}

	float PlayerController::TargetingAngle_get() const
	{
		float temp[2];
		
		m_TargetingController.GetTargetingInfo(temp + 0, temp + 1);
		
		return temp[0];
	}
	
	float PlayerController::TargetingSpreadScale_get() const
	{
		float temp[2];
		
		m_TargetingController.GetTargetingInfo(temp + 0, temp + 1);
		
		return temp[1];
	}
	
	float PlayerController::DrawingAngle_get() const
	{
		return m_MovementDirectionController.Angle_get();
	}
	
	bool PlayerController::MovementActive_get() const
	{
		return m_MovementController.IsActive_get();
	}
	
	Vec2F PlayerController::MovementDirection_get() const
	{
		Vec2F dir = m_MovementController.Direction_get();
		
		return dir;
	}
		
	float PlayerController::MovementSpeed_get() const
	{
		if (m_MovementController.IsActive_get())
		{
			//float speed = 50.0f + m_MovementController.Distance_get() * 7.5f;
			//float speed = m_MovementController.Distance_get() * 8.0f;
			//float speed = m_MovementController.Distance_get() * 8.0f * (65.0f / m_MovementController.Radius_get());
			float speed = m_MovementController.DistanceNorm_get() * 8.0f * 65.0f;
			
			if (speed > PLAYER_SPEED)
				speed = PLAYER_SPEED;
			
			return speed;
		}
		else
			return 0.0f;
	}
	
/*	bool PlayerController::SpecialActive_get() const
	{
		return m_SpecialController.IsActive_get();
	}*/
	
	bool PlayerController::TapActive_get() const
	{
		return m_TapActive;
	}
	
	void PlayerController::TapConsume()
	{
		m_TapActive = false;
	}
	
	void PlayerController::TiltCalibrate()
	{
	}
	
	void PlayerController::HandleSpawn(const Vec2F& playerPosition)
	{
		m_MovementController.DisableTouch();
		m_TargetingController.Setup(playerPosition, Calc::mPI2, 0.5f);
		
		m_TapActive = false;
	}
	
	void PlayerController::Render()
	{
		const SpriteColor color_Active = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, g_GameState->m_GameSettings->m_HudOpacity * g_World->HudOpacity_get());
		//const SpriteColor color_InActive = SpriteColor_Make(255, 255, 255, (int)(227 * g_GameState->m_GameSettings->m_HudOpacity));
		const SpriteColor color_InActive = color_Active;
		const SpriteColor color_Solid = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, g_GameState->m_GameSettings->m_HudOpacity);
		
		if (g_GameState->m_HelpState->IsCompleteOrActive(HelpState::State_HitMove))
		{
			// movement button
			
			if (m_MovementController.IsActive_get())
			{
#if !defined(PSP) && !defined(WIN32) && !defined(MACOS)
				Vec2F dir = m_MovementController.Direction_get();
				float distance = m_MovementController.Distance_get();
				if (distance > m_MovementController.Radius_get())
					distance = m_MovementController.Radius_get();
				g_GameState->Render(g_GameState->GetShape(Resources::CONTROLLER_OUTER), m_MovementController.Position_get(), 0.0f, color_Active);
				g_GameState->Render(g_GameState->GetShape(Resources::CONTROLLER_BUTTON1), m_MovementController.Position_get() + dir * distance, 0.0f, color_Solid);
#endif
			}
			else
			{
#if !defined(PSP) && !defined(WIN32) && !defined(MACOS)
				g_GameState->Render(g_GameState->GetShape(Resources::CONTROLLER_OUTER), m_MovementController.Position_get(), 0.0f, color_InActive);
				g_GameState->Render(g_GameState->GetShape(Resources::CONTROLLER_MOVE_ARROWS), m_MovementController.Position_get(), 0.0f, color_InActive);
				g_GameState->Render(g_GameState->GetShape(Resources::CONTROLLER_BUTTON1), m_MovementController.Position_get(), 0.0f, color_Solid);
#endif
			}
			
			if (g_GameState->m_HelpState->IsActive(HelpState::State_HitMove))
				RenderBouncyText(m_MovementController.Position_get() - Vec2F(0.0f, m_MovementController.Radius_get() + 30.0f), Vec2F(0.0f, 10.0f), "MOVE");
		}

		if (g_GameState->m_HelpState->IsCompleteOrActive(HelpState::State_HitFire))
		{
			// fire button
			
#if !defined(PSP) && !defined(WIN32) && !defined(MACOS)
			const SpriteColor& firingColor = m_FiringController.IsActive_get() ? color_Active : color_InActive;
			
			g_GameState->Render(g_GameState->GetShape(Resources::CONTROLLER_OUTER), m_FiringController.Position_get(), 0.0f, firingColor);
			g_GameState->Render(g_GameState->GetShape(Resources::CONTROLLER_ANGLE_DIR), m_FiringController.Position_get(), Vec2F::ToAngle(m_FiringController.Direction_get()), color_Solid);
#endif

			RenderShockwaveButton(true);
			
			if (g_GameState->m_HelpState->IsActive(HelpState::State_HitFire))
				RenderBouncyText(m_FiringController.Position_get() - Vec2F(0.0f, m_FiringController.Radius_get() + 30.0f), Vec2F(0.0f, 10.0f), "FIRE");
		}
		
		if (g_GameState->m_HelpState->IsCompleteOrActive(HelpState::State_HitSpecial))
		{
			// special gauge
			
			//RenderSpecialGauge();
//			RenderSpecialButton();
		}
	}
	
#if 0
	void PlayerController::RenderSpecialGauge()
	{
		RenderRect(m_FiringController.Position_get() + Vec2F(-24.0f, -69.0f), 1.0f, 1.0f, 1.0f, g_GameState->GetTexture(Textures::SPECIAL_GAUGE));
		
		// dynamic gauge/fill overlay
		
		SpriteGfx& gfx = g_GameState->m_SpriteGfx;
		
		const int sliceCount = 30;
		const int vertexCount = (sliceCount + 1) * 2;
		const int indexCount = sliceCount * 6;
		
		float arc = -107.0f;
		
		arc *= g_World->m_Player->SpecialAttackFill_get();
		
		const float angle1 = Calc::DegToRad(-3.0f);
		const float angle2 = angle1 + Calc::DegToRad(arc);
		
		// 50 - 65, 51 - 67
		
		const float radius1a = 51.0f;
		const float radius1b = 53.0f;
		const float radius2a = 63.0f;
		const float radius2b = 65.0f;
//		const float radius2a = 73.0f;
//		const float radius2b = 75.0f;
		
		const float x0 = m_FiringController.Position_get()[0];
		const float y0 = m_FiringController.Position_get()[1];
		
		const Atlas_ImageInfo* image = g_GameState->GetTexture(Textures::SPECIAL_SLICE);
		
		const float u1 = image->m_TexCoord[0];
		const float v1 = image->m_TexCoord[1];
		const float u2 = image->m_TexCoord2[0];
		const float v2 = image->m_TexCoord2[1];
		
		float hue1 = g_GameState->m_TimeTracker_Global.Time_get();
		float hue2 = hue1 + 0.33f;
		
		gfx.Reserve(vertexCount, indexCount);
		gfx.WriteBegin();
		
		for (int i = 0; i < sliceCount + 1; ++i)
		{
			float t = i / (float)sliceCount;
			
			float angle = angle1 + (angle2 - angle1) * t;
			
//			float alpha = 1.0f;
			float hue = hue1 + (hue2 - hue1) * t;
			
			SpriteColor color = Calc::Color_FromHue(hue);
			
			float radius1 = radius1a + (radius1b - radius1a) * t;
			float radius2 = radius2a + (radius2b - radius2a) * t;
			
			{
				const float x = x0 + cosf(angle) * radius1;
				const float y = y0 + sinf(angle) * radius1;
				gfx.WriteVertex(x, y, color.rgba, u1, v1);
			}
			{
				const float x = x0 + cosf(angle) * radius2;
				const float y = y0 + sinf(angle) * radius2;
				gfx.WriteVertex(x, y, color.rgba, u2, v2);
			}
		}
		
		int index = 0;
		
		for (int i = 0; i < sliceCount; ++i)
		{
			gfx.WriteIndex(index + 0);
			gfx.WriteIndex(index + 2);
			gfx.WriteIndex(index + 1);
			
			gfx.WriteIndex(index + 1);
			gfx.WriteIndex(index + 2);
			gfx.WriteIndex(index + 3);
			
			index += 2;
		}
		
		gfx.WriteEnd();
	}
#endif
	
	void PlayerController::RenderShockwaveButton(bool showLevel0)
	{
		const SpriteColor color_Solid = SpriteColor_MakeF(1.0f, 1.0f, 1.0f, g_GameState->m_GameSettings->m_HudOpacity);
		
		if (g_World->m_Player->ShockLevel_get() == 0)
		{
			if (showLevel0)
			{
#if !defined(PSP) && !defined(WIN32) && !defined(MACOS)
				g_GameState->Render(g_GameState->GetShape(Resources::CONTROLLER_BUTTON1), m_FiringController.Position_get(), 0.0f, color_Solid);
#endif
			}
		}
		else
			g_GameState->Render(g_GameState->GetShape(Resources::POWERUP_POWER_SHOCK), m_FiringController.Position_get(), 0.0f, color_Solid);
//		g_GameState->Render(g_GameState->GetShape(Resources::CONTROLLER_BUTTON1), m_FiringController.Position_get(), 0.0f, SpriteColors::White);
	}
	
	//
	
	bool PlayerController_DualAnalog::IsIdle_get()
	{
		return PlayerController::IsIdle_get();
	}

	float PlayerController_DualAnalog::TargetingAngle_get() const
	{
		Vec2F dir = m_FiringController.Direction_get();

		return Vec2F::ToAngle(dir);
	}
	
	float PlayerController_DualAnalog::TargetingSpreadScale_get() const
	{
		return 0.5f;
	}
	
	void PlayerController_DualAnalog::HandleSpawn(const Vec2F& playerPosition)
	{
		m_MovementController.DisableTouch();
		m_FiringController.DisableTouch();
		m_TapActive = false;
	}
	
	// Tilt Controller
	
	bool PlayerController_Tilt::IsIdle_get()
	{
		return false;
	}

	void PlayerController_Tilt::UpdateAutoAim(float dt)
	{
		mAutoAimController.Update(mPosition, dt);

		mDirection = mAutoAimController.Aim_get();
	}

	PlayerController_Tilt::PlayerController_Tilt() : PlayerController()
	{
		mCalibrationUpsideDown = false;
	}
	
	void PlayerController_Tilt::Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, float dt)
	{
		PlayerController::Update(playerPosition, playerPositionCorrection, dt);
		
#if defined(BBOS)
		bool flipped = g_System.GetScreenRotation() != 180;
#elif defined(IPAD)
		bool flipped = g_System.GetScreenRotation() != 0;
#else
		bool flipped = g_GameState->m_GameSettings->m_ScreenFlip;
#endif

		mPosition = playerPosition;
		Vec3 tilt = g_System.GetTiltVector();
		Mat4x4 matRot;
		matRot.MakeRotationY(Calc::DegToRad(flipped ? -CALIBRATION_ANGLE : +CALIBRATION_ANGLE));
		//matRot.MakeRotationY(Calc::DegToRad(+CALIBRATION_ANGLE));
		tilt = matRot.Mul(tilt);
		if (flipped)
			mTilt = Vec2F(-tilt[0], -tilt[1]);
		else
			mTilt = Vec2F(+tilt[0], +tilt[1]);
		
		if (mCalibrationUpsideDown)
			mTilt[1] = -mTilt[1];
		
#if 1
//		const float maxX = 0.25f;
		const float maxX = 0.23f;
		const float maxY = 0.19f;
		mTilt[0] /= maxX;
		mTilt[1] /= maxY;
#endif
		
		UpdateAutoAim(dt);
		
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitFire);
		g_GameState->m_HelpState->DoComplete(HelpState::State_HitMove);
	}
	
	bool PlayerController_Tilt::FireActive_get() const
	{
		/*if (g_GameState->m_GameRound->GameMode_get() == GameMode_IntroScreen || g_World->m_TouchZoomController.IsActive(ZoomTarget_ZoomedOut))
			return false;*/

		return mAutoAimController.HasTarget_get();
	}
	
	float PlayerController_Tilt::TargetingAngle_get() const
	{
		return Vec2F::ToAngle(mDirection);
	}
	
	float PlayerController_Tilt::TargetingSpreadScale_get() const
	{
		return 0.5f;
	}
	
	bool PlayerController_Tilt::MovementActive_get() const
	{
		if (g_GameState->m_GameRound->GameMode_get() == GameMode_IntroScreen || g_World->m_TouchZoomController.IsActive(ZoomTarget_ZoomedOut))
			return false;

		return true;
	}
	
	Vec2F PlayerController_Tilt::MovementDirection_get() const
	{
		Vec2F dir = mTilt.Normal();

		return Vec2F(-dir[1], -dir[0]);
	}
	
	float PlayerController_Tilt::MovementSpeed_get() const
	{
		if (g_GameState->m_GameRound->GameMode_get() == GameMode_IntroScreen || g_World->m_TouchZoomController.IsActive(ZoomTarget_ZoomedOut))
			return 0.0f;

		Vec2F tilt = mTilt;
//		tilt[0] /= maxX;
//		tilt[1] /= maxY;
		
		float speed = tilt.Length_get() * PLAYER_SPEED;
		
		if (speed > PLAYER_SPEED * 1.3f)
			speed = PLAYER_SPEED * 1.3f;
		
//		LOG_INF("speed: %03.3f", speed);
		
		return speed;
	}
	
	void PlayerController_Tilt::TiltCalibrate()
	{
		// remember current tilt angle
		
		Vec3 tilt = g_System.GetTiltVector();
		
		CALIBRATION_ANGLE = -Calc::RadToDeg(Vec2F::ToAngle(Vec2F(-tilt[2], tilt[0])));
		mCalibrationUpsideDown = false;
		
#if defined(BBOS)
		bool flipped = g_System.GetScreenRotation() != 180;
#elif defined(IPAD)
		bool flipped = g_System.GetScreenRotation() != 0;
#else
		bool flipped = g_GameState->m_GameSettings->m_ScreenFlip;
#endif

		if (flipped)
			CALIBRATION_ANGLE = -CALIBRATION_ANGLE;
		
		LOG_DBG("tilt calibrate: angle: %3.02f, upside-down: %d", CALIBRATION_ANGLE, mCalibrationUpsideDown ? 1 : 0);
	}
	
	void PlayerController_Tilt::Render()
	{
		//RenderSpecialGauge();
//		RenderSpecialButton();
		
//		if (g_GameState->m_HelpState.IsActive(HelpState::State_HitSpecial))
//			RenderBouncyText(m_SpecialController.m_Pos - Vec2F(m_SpecialController.m_Radius + 75.0f, 0.0f), Vec2F(10.0f, 0.0f), "SPECIAL");
		
		RenderShockwaveButton(false);
	}
};
