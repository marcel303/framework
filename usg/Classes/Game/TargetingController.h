#pragma once

#include "Types.h"
#include "Util_Follower.h"

namespace Game
{
	class TargetingController
	{
	public:
		TargetingController();
		void Initialize();
		
		void Setup(const Vec2F& pos, float angle, float spread);
		
		void Update(const Vec2F& playerPosition, const Vec2F& playerPositionCorrection, bool fixAngle);
		
		void GetTargetingInfo(float* out_BaseAngle, float* out_SpreadScale) const;
		
	private:
		void ConstrainPivot();
		
		GameUtil::Follower m_Follower_Position;
		GameUtil::Follower m_Follower_Pivot;
	};
}
