#include "EntityPlayer.h"
#include "GameState.h"
#include "PlayerWeapons.h"
#include "Util_ColorEx.h"
#include "World.h"

#define PLAYER g_World->m_Player

#define LASER_1_INTERVAL 3
#define LASER_2_INTERVAL 3
#define LASER_3_INTERVAL 2
#define LASER_4_INTERVAL 2
#define LASER_5_INTERVAL 1
#define LASER_6_INTERVAL 2
#define LASER_7_INTERVAL 3
#define LASER_8_INTERVAL 3
#define LASER_9_INTERVAL 2
#define LASER_10_INTERVAL 2
#define LASER_11_INTERVAL 1
#define LASER_12_INTERVAL 2

#define LASER_4_INTERVAL_ALT 30
#define LASER_5_INTERVAL_ALT 30

#define VULCAN_INTERVAL 1

#define CDC ((int)(sizeof(m_CoolDown) / sizeof(int)))

namespace Game
{
	WeaponSlot::WeaponSlot()
	{
		Initialize();
	}
	
	void WeaponSlot::Initialize()
	{
		m_Type = WeaponType_Undefined;
		m_MinLevel = 0;
		m_MaxLevel = 0;
		m_Level = 0;
		m_FireCount = 0;
		for (int i = 0; i < CDC; ++i)
			m_CoolDown[i] = 0;
	}
	
	void WeaponSlot::Setup(WeaponType type, int level)
	{
		m_Type = type;
		
		switch (type)
		{
			case WeaponType_Vulcan:
				m_MinLevel = 1;
				m_MaxLevel = 12;
				break;
			case WeaponType_Laser:
				m_MinLevel = 1;
				m_MaxLevel = 12;
				break;
				
			case WeaponType_Undefined:
			case WeaponType_ZCOUNT:
#ifndef DEPLOYMENT
				throw ExceptionNA();
#else
				break;
#endif
		}
		
		m_Level = level;
		
		ClampLevel();
	}
	
	void WeaponSlot::IncreaseLevel()
	{
		m_Level++;
		
		ClampLevel();
	}
	
	void WeaponSlot::DecreaseLevel()
	{
		m_Level--;
		
		ClampLevel();
	}
	
	void WeaponSlot::Update(bool fire)
	{
		Assert(IsActive_get());
		
		for (int i = 0; i < CDC; ++i)
		{
			m_CoolDown[i]--;
			
			if (m_CoolDown[i] < 0)
				m_CoolDown[i] = 0;
		}
		
		if (fire && m_CoolDown[0] == 0)
		{
			Fire();
		}
	}
	
	bool WeaponSlot::IsReady_get() const
	{
		return m_CoolDown == 0;
	}
	
	void WeaponSlot::Fire()
	{
		m_FireCount++;
		
		int bulletCount = 0;
		
		//
		
		Vec2F fireDirection_Forward = Vec2F::FromAngle(PLAYER->m_TargetingCone_BaseAngle);
		Vec2F fireDirection_Strafe = Vec2F::FromAngle(PLAYER->m_TargetingCone_BaseAngle + Calc::mPI2);
		Vec2F fireDirection_Strafe2 = Vec2F::FromAngle(PLAYER->Rotation_get());
		const void* id = PLAYER->IgnoreId_get();
		
#define COMMIT() g_World->SpawnBullet(bullet); bulletCount++
		
		// fire bullets!
		
		switch (Type_get())
		{
			case WeaponType_Laser:
			{
				float speed = 500.0f;
				Vec2F speedVector = Vec2F::FromAngle(PLAYER->m_TargetingCone_BaseAngle) * speed;
				SpriteColor colorRegular = SpriteColor_Make(255, 0, 0, 255);
				SpriteColor colorStrong = SpriteColor_Make(127, 0, 255, 255);
				float breadthScale = 2.0f;
				
				switch (Level_get())
				{
					case 1:
					{
						m_CoolDown[0] = LASER_1_INTERVAL;
						
						Bullet bullet;
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f, Vec2F(70.0f, breadthScale), speedVector, 10.0f, colorRegular);
						COMMIT();
						
						break;
					}
						
					case 2:
					{
						m_CoolDown[0] = LASER_2_INTERVAL;
						
						Bullet bullet;
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f - fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 6.0f, colorRegular);
						COMMIT();
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f + fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 6.0f, colorRegular);
						COMMIT();
						
						break;
					}
						
					case 3:
					{
						m_CoolDown[0] = LASER_3_INTERVAL;
						
						Bullet bullet;
						
						if (m_FireCount % 2 == 0)
						{
							bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f - fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 10.0f, colorRegular);
							COMMIT();
						}
						if (m_FireCount % 2 == 1)
						{
							bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f + fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 10.0f, colorRegular);
							COMMIT();
						}
						
						break;
					}
						
					case 4:
					{
						m_CoolDown[0] = LASER_4_INTERVAL;
						
						/*
						if (m_CoolDown[1] == 0)
						{
							m_CoolDown[1] = LASER_4_INTERVAL_ALT;
							
							FireBeam(SpriteColor_Make(255, 31, 63, 255), id);
						}*/
						
						Bullet bullet;
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f - fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 6.0f, colorRegular);
						COMMIT();
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f + fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 6.0f, colorRegular);
						COMMIT();
						
						break;
					}
						
					case 5:
					{
						m_CoolDown[0] = LASER_5_INTERVAL;
						
						/*
						if (m_CoolDown[1] == 0)
						{
							m_CoolDown[1] = LASER_5_INTERVAL_ALT;
							
							FireBeam(SpriteColor_Make(63, 31, 255, 255), id);
						}*/
						
						Bullet bullet;
						
						if (m_FireCount % 2 == 0)
						{
							bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f - fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 7.0f, colorRegular);
							COMMIT();
						}
						if (m_FireCount % 2 == 1)
						{
							bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f + fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 7.0f, colorRegular);
							COMMIT();
						}
						
						break;
					}
						
					case 6:
					{
						m_CoolDown[0] = LASER_6_INTERVAL;
						
						Bullet bullet;
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f - fireDirection_Strafe * 15.0f, Vec2F(55.0f, breadthScale), speedVector, 6.0f, colorRegular);
						COMMIT();
					
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f + fireDirection_Strafe * 15.0f, Vec2F(55.0f, breadthScale), speedVector, 6.0f, colorRegular);
						COMMIT();
						
						break;
					}
					
					// strong beam
						
					case 7:
					{
						m_CoolDown[0] = LASER_7_INTERVAL;
						
						Bullet bullet;
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f, Vec2F(70.0f, breadthScale), speedVector, 27.0f, colorStrong);
						COMMIT();
						
						break;
					}
						
					case 8:
					{
						m_CoolDown[0] = LASER_8_INTERVAL;
						
						Bullet bullet;
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f - fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 15.0f, colorStrong);
						COMMIT();
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f + fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 15.0f, colorStrong);
						COMMIT();
						
						break;
					}
						
					case 9:
					{
						m_CoolDown[0] = LASER_9_INTERVAL;
						
						Bullet bullet;
						
						if (m_FireCount % 2 == 0)
						{
							bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f - fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 22.0f, colorStrong);
							COMMIT();
						}
						if (m_FireCount % 2 == 1)
						{
							bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f + fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 22.0f, colorStrong);
							COMMIT();
						}
						
						break;
					}
						
					case 10:
					{
						m_CoolDown[0] = LASER_10_INTERVAL;
												
						Bullet bullet;
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f - fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 12.0f, colorStrong);
						COMMIT();
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f + fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 12.0f, colorStrong);
						COMMIT();
						
						break;
					}
						
					case 11:
					{
						m_CoolDown[0] = LASER_11_INTERVAL;

						Bullet bullet;
						
						if (m_FireCount % 2 == 0)
						{
							bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f - fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 13.0f, colorStrong);
							COMMIT();
						}
						if (m_FireCount % 2 == 1)
						{
							bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f + fireDirection_Strafe * 15.0f, Vec2F(70.0f, breadthScale), speedVector, 13.0f, colorStrong);
							COMMIT();
						}
						
						break;
					}
					
					case 12:
					{
						m_CoolDown[0] = LASER_12_INTERVAL;
						
						SpriteColor color = Calc::Color_FromHue(g_GameState->m_TimeTracker_World->Time_get());
						
						Bullet bullet;
						
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f - fireDirection_Strafe * 15.0f, Vec2F(55.0f, breadthScale), speedVector, 14.0f, color);
						COMMIT();
					
						bullet.MakeBeamSegment(id, PLAYER->Position_get() + fireDirection_Forward * 2.0f + fireDirection_Strafe * 15.0f, Vec2F(55.0f, breadthScale), speedVector, 14.0f, color);
						COMMIT();
						
						break;
					}
				}
				break;
			}
				
			case WeaponType_Vulcan:
			{
				FireVulcan(bulletCount, fireDirection_Forward, fireDirection_Strafe, fireDirection_Strafe2, id);
				break;
			}
				
			case WeaponType_Undefined:
			case WeaponType_ZCOUNT:
/*#ifndef DEPLOYMENT
				throw ExceptionNA();
#else*/
				break;
//#endif
		}
		
		g_World->m_Player->m_Stat_BulletCount += bulletCount;
	}
	
	void WeaponSlot::FireVulcan(int& bulletCount, Vec2F fireDirection_Forward, Vec2F fireDirection_Strafe, Vec2F fireDirection_Strafe2, const void* id)
	{
		const Vec2F origin = PLAYER->Position_get();
		const float speed = 500.0f;
		const float frontDistance = 5.0f;
		const float bulletSize = 25.0f;
		
		m_CoolDown[0] = VULCAN_INTERVAL;
		
		float angles[5];
		PLAYER->GetTargetingAngles_Random(5, angles);
		
		Bullet bullet;
		
		switch (Level_get())
		{
			case 1:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize, fireDirection_Forward * speed, VulcanType_Regular, 1.0f);
				COMMIT();
				break;
			}
				
			case 2:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize + fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Regular, 0.6f);
				COMMIT();

				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize - fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Regular, 0.6f);
				COMMIT();
				break;
			}
				
			case 3:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize + fireDirection_Strafe * frontDistance * 2.0f, fireDirection_Forward * speed, VulcanType_Regular, 0.4f);
				COMMIT();

				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize, fireDirection_Forward * speed, VulcanType_Regular, 0.4f);
				COMMIT();
				
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize - fireDirection_Strafe * frontDistance * 2.0f, fireDirection_Forward * speed, VulcanType_Regular, 0.4f);
				COMMIT();
				break;
			}
				
			case 4:
			{
				for (int j = 0; j < 3; ++j)
				{
					const Vec2F dir = Vec2F::FromAngle(angles[j]);
					bullet.MakeVulcan(id, origin + dir * bulletSize, dir * speed, VulcanType_Regular, 0.5f);
					COMMIT();
				}
				break;
			}
				
			case 5:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize + fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Regular, 0.6f);
				COMMIT();

				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize - fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Regular, 0.6f);
				COMMIT();
				
				for (int j = 0; j < 2; ++j)
				{
					const Vec2F dir = Vec2F::FromAngle(angles[j]);
					bullet.MakeVulcan(id, origin + dir * bulletSize, dir * speed, VulcanType_Regular, 0.4f);
					COMMIT();
				}
				break;
			}
				
			case 6:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize + fireDirection_Strafe * frontDistance * 2.0f, fireDirection_Forward * speed, VulcanType_Regular, 0.4f);
				COMMIT();

				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize, fireDirection_Forward * speed, VulcanType_Regular, 0.4f);
				COMMIT();
				
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize - fireDirection_Strafe * frontDistance * 2.0f, fireDirection_Forward * speed, VulcanType_Regular, 0.4f);
				COMMIT();
				
				for (int j = 0; j < 2; ++j)
				{
					const Vec2F dir = Vec2F::FromAngle(angles[j]);
					bullet.MakeVulcan(id, origin + dir * bulletSize, dir * speed, VulcanType_Regular, 0.4f);
					COMMIT();
				}
				break;
			}
				
			case 7:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize + fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Regular, 0.6f);
				COMMIT();

				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize - fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Regular, 0.6f);
				COMMIT();
				
				for (int j = 0; j < 3; ++j)
				{
					const Vec2F dir = Vec2F::FromAngle(angles[j]);
					bullet.MakeVulcan(id, origin + dir * bulletSize, dir * speed, VulcanType_Regular, 0.4f);
					COMMIT();
				}
				break;
			}
				
			// strong vulcan
				
			case 8:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize + fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Upgraded, 1.3f);
				COMMIT();

				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize - fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Upgraded, 1.3f);
				COMMIT();
				break;
			}
				
			case 9:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize, fireDirection_Forward * speed, VulcanType_Upgraded, 0.9f);
				COMMIT();
				
				for (int j = 0; j < 2; ++j)
				{
					const Vec2F dir = Vec2F::FromAngle(angles[j]);
					bullet.MakeVulcan(id, origin + dir * bulletSize, dir * speed, VulcanType_Upgraded, 0.9f);
					COMMIT();
				}
				break;
			}
				
			case 10:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize + fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Upgraded, 0.7f);
				COMMIT();
				
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize - fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Upgraded, 0.7f);
				COMMIT();
				
				for (int j = 0; j < 2; ++j)
				{
					const Vec2F dir = Vec2F::FromAngle(angles[j]);
					bullet.MakeVulcan(id, origin + dir * bulletSize, dir * speed, VulcanType_Upgraded, 0.7f);
					COMMIT();
				}
				break;
			}
				
			case 11:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize + fireDirection_Strafe * frontDistance * 2.0f, fireDirection_Forward * speed, VulcanType_Upgraded, 0.6f);
				COMMIT();

				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize, fireDirection_Forward * speed, VulcanType_Upgraded, 0.6f);
				COMMIT();
				
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize - fireDirection_Strafe * frontDistance * 2.0f, fireDirection_Forward * speed, VulcanType_Upgraded, 0.6f);
				COMMIT();
				
				for (int j = 0; j < 2; ++j)
				{
					const Vec2F dir = Vec2F::FromAngle(angles[j]);
					bullet.MakeVulcan(id, origin + dir * bulletSize, dir * speed, VulcanType_Upgraded, 0.6f);
					COMMIT();
				}
				break;
			}

			case 12:
			{
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize + fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Upgraded, 0.6f);
				COMMIT();
				
				bullet.MakeVulcan(id, origin + fireDirection_Forward * bulletSize - fireDirection_Strafe * frontDistance, fireDirection_Forward * speed, VulcanType_Upgraded, 0.6f);
				COMMIT();
				
				for (int j = 0; j < 4; ++j)
				{
					const Vec2F dir = Vec2F::FromAngle(angles[j]);
					bullet.MakeVulcan(id, origin + dir * bulletSize, dir * speed, VulcanType_Upgraded, 0.6f);
					COMMIT();
				}
				break;
			}
				
			default:
				break;
		}
	}
	
	void WeaponSlot::FireBeam(SpriteColor color, const void* ignoreId)
	{
		LaserBeam* beam = PLAYER->m_LaserBeamMgr.AllocateBeam();
		
		if (!beam)
			return;
		
		beam->Setup(0.0f, true, 300.0f, 1.8f, 0.0f, 0.0f, Calc::DegToRad(360.0f), 0.5f, 2.0f, 0.25f, 20.0f, color, ignoreId);
	}	
	
	WeaponType WeaponSlot::Type_get() const
	{
		return m_Type;
	}
	
	int WeaponSlot::Level_get() const
	{
		return m_Level;
	}
	
	void WeaponSlot::Level_set(int value)
	{
		m_Level = value;
		
		ClampLevel();
	}
	
	int WeaponSlot::MaxLevel_get() const
	{
		return m_MaxLevel;
	}
	
	bool WeaponSlot::IsActive_get() const
	{
		if (m_Level == 0)
			return false;
		
		return true;
	}
	
	void WeaponSlot::ClampLevel()
	{
		if (m_Level < m_MinLevel)
			m_Level = m_MinLevel;
		if (m_Level > m_MaxLevel)
			m_Level = m_MaxLevel;
	}
}
