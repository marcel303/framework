#pragma once

#include "Entity.h"
#include "Forward.h"
#include "MissileTrail.h"
#include "SpriteGfx.h"

namespace Game
{
	enum BulletType
	{
		BulletType_Undefined,
		BulletType_Vulcan,
		BulletType_VulcanSTR, // strong vulcan
		BulletType_VulcanBOSS, // boss vulcan
		BulletType_VulcanINV, // invader vulcan
		BulletType_Maxi_BlueSpray, // maxi boss spray weapon (omni)
		BulletType_Maxi_PurpleSpray, // maxi boss spray weapon (directional)
		BulletType_Missile,
		BulletType_LaserSegment
	};
	
	enum VulcanType
	{
		VulcanType_Regular,
		VulcanType_Upgraded,
		VulcanType_Strong,
		VulcanType_Boss,
		VulcanType_InvaderFire
	};
	
	enum BulletSound
	{
		BulletSound_Fire,
		BulletSound_Impact
	};
	
	/*enum BullletFlags
	{
		BulletFlag_IsAllocated = 0x01,
		BulletFlag_IsDead = 0x02,
		BulletFlag_NoClip = 0x03
	};*/
	
	typedef CD_TYPE (*BulletMissileTargetSelectCB)(class Bullet* bullet, void* obj);
	
	class Bullet
	{
	public:
		Bullet();
		void Initialize();
		
		void Update(float dt);
		void UpdateCollision(float dt);
		void Render();
		void Render_Additive();
		void HandleHit(const Vec2F& pos, Entity* hitEntity, float dt);
		
		void MakeVulcan(const void* ignore, const Vec2F& pos, const Vec2F& vel, VulcanType type, float damage);
		void MakeMissile(const void* ignore, const Vec2F& pos, const Vec2F& vel, BulletMissileTargetSelectCB targetCb, void* targetObj, float huntTime);
		void MakeBeamSegment(const void* ignore, const Vec2F& pos, const Vec2F& size, const Vec2F& vel, float damage, SpriteColor color);
		void MakeMaxiBlueSpray(const void* ignore, const Vec2F& pos, const Vec2F& vel, float damage);
		void MakeMaxiPurpleSpray(const void* ignore, const Vec2F& pos, const Vec2F& vel, float damage);

		void PlaySound(BulletSound sound);
		
//	private:
		void MakeShared(BulletType type, const void* ignore, float energy, float damage, const Vec2F& pos, const Vec2F& vel, const VectorShape* vectorShape);
		void MissileTargetSelect();
		
		// Management
		XBOOL m_IsAllocated;
		XBOOL m_IsDead;
/*		int m_Flags;
		
		inline void Flag_set(BulletFlag flag)
		{
			m_Flags |= flag;
		}
		
		inline void Flag_reset(BulletFlag flag)
		{
			m_Flags &= ~flag;
		}
		
		inline int Flag_isset(BulletFlag flag) const
		{
			return m_Flags & flag;
		}*/
		
		// Logic
		BulletType m_Type;
		const void* m_Ignore;
		float m_Energy;
		float m_Damage;
		Vec2F m_Pos;
		Vec2F m_Vel;
		
		// Drawing
		const VectorShape* m_VectorShape;
		bool m_Reflected;
		bool m_IsCaptured;
		
		// Hunt specific
		CD_TYPE m_TargetId;
		Vec2F m_TargetPos;
		
		// Missile specific
		BulletMissileTargetSelectCB m_MissileTargetCb;
		void* m_MissileTargetObj;
		MissileTrail m_MissileTrail;
		float m_MissileHuntTime;
		float m_MissileTargetDelay;
		
		// Beam segment specific
		Vec2F m_BeamDelta;
		Vec2F m_BeamSize;
		float m_BeamLength;
		float m_BeamGrowSpeed;
		SpriteColor m_BeamColor;
	};
}
