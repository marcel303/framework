#pragma once

#include "AngleController.h"
#include "AnimTimer.h"
#include "BanditSeq.h"
#include "CompiledComposition.h"
#include "Entity.h"
#include "FireController.h"
#include "Forward.h"
#include "libgg_forward.h"
#include "LimitedPulseTimer.h"
#include "Mat3x2.h"
#include "MaxiLaserBeam.h"
#include "Types.h"

namespace Bandits
{	
	class EntityBandit;
	class Link;
	class Segment;
	class Bandit;
	class Reader;
	
	enum VulcanPattern
	{
		VulcanPattern_Targeted, // vulcan fire directly targeted at player - spawn many, give up
		VulcanPattern_CirclePulse, // vulcan fire circle, immediate release - for best effect, repeat 3x or so
		VulcanPattern_CircleRotate, // vulcan fire circle, rotating - spawn at least 1 1/3 circle for best effect
		VulcanPattern__Count
	};
	
	enum Weapon
	{
		Weapon_Undefined,
		Weapon_Vulcan,
		Weapon_Missile,
		Weapon_Beam,
		Weapon_BlueSpray,
		Weapon_PurpleSpray
	};
	
	enum LinkType
	{
		LinkType_Undefined,
		LinkType_Segment,
		LinkType_Core,
		LinkType_Weapon
	};
	
	class Link : public Game::Entity
	{
	public:
		Link();
		virtual ~Link();
		virtual void Initialize();
		
		void Setup(Bandit* bandit, Link* parent, Vec2F pos, float baseAngle, float angle, float minAngle, float maxAngle, float angleSpeed, VectorShape* shape, bool mirrorX, bool mirrorY, int flags);
		void Finalize();
		
		virtual void Update(float dt);
		virtual void UpdateSB(SelectionBuffer* sb);
		
		int ActivateBeamAttack();
		
		Link* Child_get(int index);
		void Child_set(int index, Link* link);
		void ChildCount_set(int childCount);
		
		LinkType LinkType_get() const;
		Weapon WeaponType_get() const;
		
	public:
		virtual void Render_Below();
		virtual void Render();
		void DoRender();
		
		virtual void HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, Game::DamageType type);
		virtual void HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, Game::DamageType type);
		virtual void HandleDie();
		void HandleDeath(Link* link);
		void DoDestroy();
		
	private:
		void Allocate(int childCount);
		void InsulationTraverseUp(int delta);
		void IsDeadTraverseDown();
//		bool IsRootLink_get();
		EntityBandit* Entity_get();
		
		Bandit* m_Bandit;
		Link* m_Parent;
		Link** m_Links;
		int m_LinkCount;
		int m_InsulationFactor;
		float m_BaseAngle;
		float m_MinAngle;
		float m_MaxAngle;
		float m_VarAngle;
		float m_AngleSpeed;
		bool m_MirrorX;
		bool m_MirrorY;
		int m_LinkFlags;
		LinkType m_LinkType;
//		bool m_IsWeapon;
		
		// --------------------
		// Transform
		// --------------------
		void UpdateRotation(float dt);
		void UpdateTransform();
	public:
		inline const Vec2F& GlobalPosition_get() const
		{
			return m_GlobalPosition;
		}
		inline float GlobalRotation_get() const
		{
			return m_GlobalRotation;
		}
		
	private:
		Vec2F m_LocalPosition;
		Vec2F m_GlobalPosition;
		float m_GlobalRotation;
		Mat3x2 m_GlobalTransform;
		
		// --------------------
		// Rotation
		// --------------------
		AngleController m_AngleController;
		
		// --------------------
		// Weapons
		// --------------------
		void UpdateWeapon(float dt);
	public:
		float WeaponAngle_get() const;
	private:
		bool BeamEnabled_get() const;
		Vec2F WeaponTargetDir_get() const;
		void HandleFire();
		void FireVulcan(VulcanPattern pattern);
		int GetVulcanRepeatCount() const;
		
		Weapon m_Weapon;
		bool m_WeaponTarget;
		int m_BeamActivationCount;
		TriggerTimerW m_AttackTrigger;
		TriggerTimerW m_AttackLoadTrigger;
		Game::LimitedPulseTimer m_FireTimer;
		AngleController m_Weapon_AngleController;
		VulcanPattern m_VulcanPattern;
		float m_VulcanPattern_Angle;
		
		// --------------------
		// Panic attack
		// --------------------
		void UpdatePanicAttack(float dt);
		void BeginPanicAttack();
		
		TriggerTimerW m_PanicAttackTrigger;
		Game::LimitedPulseTimer m_PanicFireTimer;
		VulcanPattern m_PanicPattern;
		
		// --------------------
		// Animation
		// --------------------
		AnimTimer m_HitTimer;
		
		// --------------------
		// Drawing
		// --------------------
		const VectorShape* m_Shape;
		
		friend class Bandit;
		friend class Converter;
		friend class EntityBandit;
		friend class Game::MaxiLaserBeam;
	};
	
	// bandit, maintains shared bandit state, but does not control behaviour
	class Bandit
	{
	public:
		Bandit();
		~Bandit();
		void Initialize();
		
		void Setup(EntityBandit* entity, Link** links, int linkCount, Link* rootLink);
		void Finalize();
		
		void Update(float dt);
		void UpdateSB(SelectionBuffer* sb);
		void Render_Below();
		void Render();
		
		// --------------------
		// Links
		// --------------------
		Link* RootLink_get();
		Link** LinkArray_get();
		int LinkArraySize_get();
		uint32_t Insulation_get() { return m_RootLink->m_InsulationFactor; }
		
	private:
		// --------------------
		// Laser beam attack
		// --------------------
		Game::MaxiLaserBeam* AllocateBeam();
		void StopBeamAttack();
		bool StartBeamAttack();
		
		Game::MaxiLaserBeamMgr m_LaserBeamMgr;
		
		// --------------------
		// Links
		// --------------------
		EntityBandit* m_Entity;
		Link* m_RootLink;
		Link** m_Links;
		int m_LinkCount;
		
		friend class EntityBandit;
		friend class Link;
	};
}
