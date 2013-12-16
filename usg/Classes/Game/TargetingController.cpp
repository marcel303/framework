#include "Calc.h"
#include "TargetingController.h"

#define MIN_DISTANCE 0.0f
#define MAX_DISTANCE 80.0f

namespace Game
{
	TargetingController::TargetingController()
	{
		Initialize();
	}
	
	void TargetingController::Initialize()
	{
		m_Follower_Position.Setup(0, 0.0f, 0.0f, Vec2F(0.0f, 0.0f));
		m_Follower_Pivot.Setup(&m_Follower_Position, MIN_DISTANCE, MAX_DISTANCE, Vec2F(0.0f, -100.0f));
	}
	
	void TargetingController::Setup(const Vec2F& pos, float angle, float spread)
	{
		const float distance = Calc::Lerp(MIN_DISTANCE, MAX_DISTANCE, spread);
		
		m_Follower_Position.m_Position = pos;
		m_Follower_Pivot.m_Position = m_Follower_Position.m_Position + Vec2F::FromAngle(angle) * distance;
	}
	
	void TargetingController::Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, bool fixAngle)
	{
		Vec2F delta = playerPosition - m_Follower_Position.m_Position;
		
		m_Follower_Position.m_Position = playerPosition;
		
		if (fixAngle)
		{
			m_Follower_Pivot.m_Position += delta;
		}
		else
		{
			m_Follower_Pivot.m_Position += playerPositionCorrection;
		}
		
		ConstrainPivot();
	}
	
	void TargetingController::GetTargetingInfo(float* out_BaseAngle, float* out_SpreadScale) const
	{
		Vec2F delta = -m_Follower_Pivot.Delta_get();
		
		*out_BaseAngle = Vec2F::ToAngle(delta);
		
		float distance = delta.Length_get();
		
		*out_SpreadScale = 1.0f - (distance - MIN_DISTANCE) / (MAX_DISTANCE - MIN_DISTANCE);
	}
	
	void TargetingController::ConstrainPivot()
	{
		m_Follower_Pivot.Solve();
	}
};
