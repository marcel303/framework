#include "LaserBeamMgr.h"

namespace Game
{
	LaserBeamMgr::LaserBeamMgr()
	{
		m_ControlAngle = 0.0f;
	}
	
	void LaserBeamMgr::Update(float dt)
	{
		for (int i = 0; i < MAX_LASER_BEAMS; ++i)
		{
			if (!m_Beams[i].IsActive_get())
				continue;
			
			if (m_Beams[i].m_AngleControl)
				m_Beams[i].m_Angle = m_ControlAngle;
			
			m_Beams[i].Update(dt);
		}
	}
	
	void LaserBeamMgr::Render()
	{
		for (int i = 0; i < MAX_LASER_BEAMS; ++i)
		{
			if (!m_Beams[i].IsActive_get())
				continue;
			
			m_Beams[i].Render();
		}
	}
	
	LaserBeam* LaserBeamMgr::AllocateBeam()
	{
		for (int i = 0; i < MAX_LASER_BEAMS; ++i)
		{
			if (!m_Beams[i].IsActive_get())
				return &m_Beams[i];
		}
		
		return 0;
	}
	
	void LaserBeamMgr::RemoveAll()
	{
		for (int i = 0; i < MAX_LASER_BEAMS; ++i)
		{
			if (m_Beams[i].IsActive_get())
				return m_Beams[i].Stop();
		}
	}
	
	void LaserBeamMgr::Position_set(const Vec2F& pos)
	{
		for (int i = 0; i < MAX_LASER_BEAMS; ++i)
		{
			m_Beams[i].Position_set(pos);
		}
	}
	
	void LaserBeamMgr::ControlAngle_set(float angle)
	{
		m_ControlAngle = angle;
	}
}
