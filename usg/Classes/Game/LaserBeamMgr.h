#pragma once

#include "LaserBeam.h"
#include "Types.h"

namespace Game
{
	class LaserBeamMgr
	{
	public:
		LaserBeamMgr();
		
		void Update(float dt);
		void Render();
		
		LaserBeam* AllocateBeam();
		void RemoveAll();
		
		void Position_set(const Vec2F& pos);
		void ControlAngle_set(float angle);
		
	private:
		LaserBeam m_Beams[MAX_LASER_BEAMS];
		float m_ControlAngle;
	};
}
