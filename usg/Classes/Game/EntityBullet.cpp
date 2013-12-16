#include "Camera.h"
#include "EntityBullet.h"
#include "EntityPlayer.h"
#include "GameRound.h"
#include "GameSettings.h"
#include "GameState.h"
#include "SoundEffectMgr.h"
#include "Textures.h"
#include "TempRender.h"
#include "UsgResources.h"
#include "Util_Color.h"
#include "World.h"

//#define MISSILE_SPEED 150.0f
#define MISSILE_SPEED 200.0f

namespace Game
{
	Bullet::Bullet()
	{
		Initialize();
	}
	
	void Bullet::Initialize()
	{
		// Management
		m_IsAllocated = XFALSE;
		m_IsDead = XTRUE;
//		m_Flags = BulletFlag_IsDead;
		
		// Logic
		m_Type = BulletType_Undefined;
		m_Ignore = 0;
		m_Energy = 0.0f;
		m_Damage = 0.0f;
		m_Pos.SetZero();
		m_Vel.SetZero();
		
		// Drawing
		m_VectorShape = 0;
		m_Reflected = false;
		m_IsCaptured = false;
		
		// Hunt
		m_TargetId = 0;
		
		// Missile
		m_MissileTargetCb = 0;
		m_MissileTargetObj = 0;
		m_MissileHuntTime = 0.0f;
		
		// Beam
		m_BeamLength = 0.0f;
		m_BeamGrowSpeed = 0.0f;
	}
	
	void Bullet::Update(float dt)
	{
		switch (m_Type)
		{
			case BulletType_Vulcan:
			case BulletType_VulcanSTR:
			case BulletType_Maxi_PurpleSpray:
			{
				// vulcan bullets die when outside of the screen area
				
				if (!g_GameState->m_Camera->m_Area.IsInside(m_Pos))
					m_IsDead = XTRUE;
				
				break;
			}
				
			case BulletType_VulcanBOSS:
			case BulletType_VulcanINV: // todo: query screen + border for culling
			case BulletType_Maxi_BlueSpray:
				break;
				
			case BulletType_Missile:
			{
				// adjust velocity based on target
				
				m_MissileTargetDelay -= dt;
				
				if (m_MissileTargetDelay < 0.0f)
					m_MissileTargetDelay = 0.0f;
				
				if (m_MissileTargetDelay == 0.0f && m_MissileHuntTime >= 0.0f)
				{
					if (m_TargetId != 0 && g_EntityInfo[m_TargetId].alive)
					{
						m_TargetPos = g_EntityInfo[m_TargetId].position;
						
						m_MissileHuntTime -= dt;
						
						Vec2F delta = m_TargetPos - m_Pos;
						
						delta.Normalize();
						
//						m_Vel += delta * 400.0f * dt;
						m_Vel += delta * 1000.0f * dt;
					}
					else
					{
						MissileTargetSelect();
						
						if (m_TargetId == 0)
						{
							m_MissileHuntTime = 0.0f;
						}
					}
				}
				
				m_Vel.Normalize();
				
				m_Vel *= MISSILE_SPEED;
				
				//
				
				m_MissileTrail.Position_set(m_Pos);
				m_MissileTrail.Update(dt);

				break;
			}
				
			case BulletType_LaserSegment:
			{
				// laser segments die when outside of the screen area
				
				if (!g_GameState->m_Camera->m_Area.IsInside(m_Pos))
					m_IsDead = XTRUE;
				
				// expand laser beam size
				
				m_BeamLength += m_BeamGrowSpeed * dt;
				
				if (m_BeamLength > m_BeamSize[0])
					m_BeamLength = m_BeamSize[0];
				
				if (m_BeamLength < m_BeamSize[0])
					m_Vel.SetZero();
				else
					m_Vel = m_BeamDelta * m_BeamGrowSpeed;
				
				break;
			}
				
			case BulletType_Undefined:
#ifndef DEPLOYMENT
				throw ExceptionVA("bullet type undefined");
#else
				break;
#endif
		}
		
		m_Pos += m_Vel * dt;
		
		UpdateCollision(dt);
		
		m_Energy -= dt;
		
		if (m_Energy <= 0.0f)
		{
			m_IsDead = XTRUE;
			m_Energy = 0.0f;
		}
		
		if (m_Pos[0] < 0.0f || m_Pos[1] < 0.0f || m_Pos[0] >= WORLD_SX || m_Pos[1] >= WORLD_SY)
		{
			m_IsDead = XTRUE;
		}
	}
	
	void Bullet::UpdateCollision(float dt)
	{
		switch (m_Type)
		{
			case BulletType_LaserSegment:
			{
				void* hit[10];
				Vec2F pos[10];
				const int hitCount = g_GameState->m_SelectionMap.Query_Line(&g_GameState->m_SelectionBuffer, m_Pos, m_Pos + m_BeamDelta * m_BeamLength, 3.0f, hit, pos, 10);
				
				for (int i = 0; i < hitCount; ++i)
				{
					Entity* entity = (Entity*)hit[i];
					
					HandleHit(pos[i], entity, dt);
				}
				break;
			}
				
			case BulletType_VulcanINV:
			{
				// query selection buffer
				
				Entity* hit = (Entity*)g_GameState->m_SelectionMap.Query_Point(&g_GameState->m_SelectionBuffer, m_Pos);
				
				if (hit == (Entity*)g_World->m_Player)
				{
					HandleHit(m_Pos, hit, dt);
				}
				
				break;
			}
				
			default:
			{
				// query selection buffer
				
				Entity* hit = (Entity*)g_GameState->m_SelectionMap.Query_Point(&g_GameState->m_SelectionBuffer, m_Pos);
				
				if (hit)
				{
					HandleHit(m_Pos, hit, dt);
				}
				
				break;
			}
		}
	}
	
	void Bullet::Render()
	{
		if (m_Reflected)
		{
			Vec2F p1 = g_World->m_Player->Position_get();
			Vec2F p2 = m_Pos;
			Vec2F pd = p2 - p1;
			float len = pd.Length_get();
			float v = 1.0f - len / 200.0f;
			if (v > 0.0f)
			{
				int a = Calc::Min(int(v * 255.0f), 255);
				SpriteColor c = SpriteColor_Make(15, 31, 0, a);
				RenderCurve(p1, Vec2F(), p2, Vec2F(), 2.0f, c, 2);
			}
		}
		
		switch (m_Type)
		{
			case BulletType_LaserSegment:
			{
				break;
			}
				
			case BulletType_VulcanBOSS:
			case BulletType_VulcanINV:
			{
				const float rotation = Vec2F::ToAngle(m_Vel);
				
				uint8_t v = g_GameState->m_GameRound->RoundThemeColor_get().v[1];
				SpriteColor c = SpriteColor_Make(v, v , v, 255);
				
				g_GameState->Render(m_VectorShape, m_Pos, rotation, c);
				
				break;
			}
				
			default:
			{
				const float rotation = Vec2F::ToAngle(m_Vel);
				
				g_GameState->Render(m_VectorShape, m_Pos, rotation, SpriteColors::Black);
				
				break;
			}
		}
	}
	
	void Bullet::Render_Additive()
	{
		switch (m_Type)
		{
			case BulletType_LaserSegment:
			{
				const float scale = m_BeamSize[1];
				
				RenderBeamEx(scale, m_Pos, m_Pos + m_BeamDelta * m_BeamLength, m_BeamColor,
						   g_GameState->GetTexture(Textures::BEAM_02_BACK_CORNER1),
						   g_GameState->GetTexture(Textures::BEAM_02_BACK_CORNER2),
						   g_GameState->GetTexture(Textures::BEAM_02_BACK_BODY), 1);
				
				RenderBeamEx(scale, m_Pos, m_Pos + m_BeamDelta * m_BeamLength, SpriteColors::White,
							 g_GameState->GetTexture(Textures::BEAM_02_CORE_CORNER1),
							 g_GameState->GetTexture(Textures::BEAM_02_CORE_CORNER2),
						   g_GameState->GetTexture(Textures::BEAM_02_CORE_BODY), 1);
				break;
			}
				
			default:
			{
				break;
			}
		}
	}
	
	void Bullet::HandleHit(const Vec2F& pos, Entity* hitEntity, float dt)
	{
		if (hitEntity->IgnoreId_get() == m_Ignore)
			return;
		
		float damage = m_Damage;
		
		DamageType damageType = DamageType_Instant;
		
		if (m_Type == BulletType_LaserSegment)
		{
			damageType = DamageType_OverTime;
			damage *= dt;
		}
		else
		{
			m_IsDead = XTRUE;
		}
		
		hitEntity->HandleDamage(pos, m_Vel, damage, damageType);
	}
	
	void Bullet::MakeVulcan(const void* ignore, const Vec2F& pos, const Vec2F& vel, VulcanType type, float damage)
	{
		BulletType bulletType = BulletType_Undefined;
		int shapeId = -1;
		
		switch (type)
		{
			case VulcanType_Regular:
			{
				bulletType = BulletType_Vulcan;
				shapeId = Resources::BULLET_VULCAN;
				break;
			}
			case VulcanType_Upgraded:
			{
				bulletType = BulletType_VulcanSTR;
				shapeId = Resources::BULLET_VULCAN_STRONG;
				break;
			}
			case VulcanType_Strong:
			{
				bulletType = BulletType_VulcanSTR;
				shapeId = Resources::BULLET_VULCAN_STRONG;
				break;
			}
			case VulcanType_Boss:
			{
				bulletType = BulletType_VulcanBOSS;
				shapeId = Resources::BULLET_VULCAN_MAXI;
				break;
			}
			case VulcanType_InvaderFire:
			{
				bulletType = BulletType_VulcanINV;
				shapeId = Resources::BULLET_VULCAN_MAXI;
				break;
			}
				
			default:
			{
#if DEBUG
				throw ExceptionVA("not implemented");
#else
				bulletType = BulletType_Vulcan;
				shapeId = Resources::PLAYER_SHIP;
				break;
#endif
			}
		}
		
		MakeShared(bulletType, ignore, BULLET_DEFAULT_LIFE, damage, pos, vel, g_GameState->GetShape(shapeId));
	}
	
	void Bullet::MakeMissile(const void* ignore, const Vec2F& pos, const Vec2F& vel, BulletMissileTargetSelectCB targetCb, void* targetObj, float huntTime)
	{
		MakeShared(BulletType_Missile, ignore, 10.0f, 5.0f, pos, vel, g_GameState->GetShape(Resources::MISSILE));
		
		m_MissileTargetCb = targetCb;
		m_MissileTargetObj = targetObj;
		m_MissileHuntTime = huntTime;
		m_MissileTargetDelay = 0.3f;
		m_MissileTrail.Setup(pos);
		
		MissileTargetSelect();
	}
	
	void Bullet::MakeBeamSegment(const void* ignore, const Vec2F& pos, const Vec2F& size, const Vec2F& vel, float damage, SpriteColor color)
	{
		MakeShared(BulletType_LaserSegment, ignore, 1000000.0f, damage, pos, vel, 0);
		
		m_BeamSize = size;
		m_BeamDelta = vel.Normal();
		m_BeamLength = 0.0f;
		m_BeamGrowSpeed = vel.Length_get();
		m_BeamColor = color;
	}
	
	void Bullet::MakeMaxiBlueSpray(const void* ignore, const Vec2F& pos, const Vec2F& vel, float damage)
	{
		MakeShared(BulletType_Maxi_BlueSpray, ignore, 1000000.0f, damage, pos, vel, g_GameState->GetShape(Resources::BULLET_MAXI_BLUE));
	}
	
	void Bullet::MakeMaxiPurpleSpray(const void* ignore, const Vec2F& pos, const Vec2F& vel, float damage)
	{
		MakeShared(BulletType_Maxi_PurpleSpray, ignore, 1000000.0f, damage, pos, vel, g_GameState->GetShape(Resources::BULLET_MAXI_PURPLE));
	}
	
	static int GetSound(BulletType type, BulletSound sound)
	{
		switch (type)
		{
			case BulletType_Vulcan:
			case BulletType_VulcanSTR:
			case BulletType_VulcanBOSS:
			case BulletType_VulcanINV:
				if (sound == BulletSound_Fire)
					return Resources::SOUND_HIT_01;
				if (sound == BulletSound_Impact)
					return 0;
				break;
				
			case BulletType_Missile:
				if (sound == BulletSound_Fire)
					return Resources::SOUND_MISSILE_LAUNCH;
				if (sound == BulletSound_Impact)
					//return Resources::SOUND_EXPLODE_SOFT;
					return 0;
				break;
				
			case BulletType_LaserSegment:
				if (sound == BulletSound_Fire)
					return Resources::SOUND_PLAYER_LASER_SHORT;
				if (sound == BulletSound_Impact)
					return 0;
				break;
				
			case BulletType_Maxi_BlueSpray:
			case BulletType_Maxi_PurpleSpray:
			case BulletType_Undefined:
				return 0;
		}
		
		return 0;
	}
	
	void Bullet::PlaySound(BulletSound sound)
	{
		int resourceId = GetSound(m_Type, sound);
		
		if (resourceId != 0)
		{
			g_GameState->m_SoundEffects->Play(resourceId, 0);
		}
	}
	
	void Bullet::MakeShared(BulletType type, const void* ignore, float energy, float damage, const Vec2F& pos, const Vec2F& vel, const VectorShape* vectorShape)
	{
		m_Type = type;
		m_Ignore = ignore;
		m_Energy = energy;
		m_Damage = damage;
		m_Pos = pos;
		m_Vel = vel;
		m_IsDead = XFALSE;
		m_VectorShape = vectorShape;
	}
	
	void Bullet::MissileTargetSelect()
	{
		m_TargetId = m_MissileTargetCb(this, m_MissileTargetObj);
	}
}
