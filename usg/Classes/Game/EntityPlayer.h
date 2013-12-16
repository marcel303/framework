#pragma once

#include "Anim_ScreenFlash.h"
#include "AnimTimer.h"
#include "Entity.h"
#include "EntityPowerup.h"
#include "EntityPowerball.h"
#include "Fader.h"
#include "Forward.h"
#include "GameTypes.h"
#include "Gauge.h"
#include "InvincibilityTimer.h"
#include "LaserBeamMgr.h"
#include "Log.h"
#include "PlayerWeapons.h"
#include "PolledTimer.h"
#include "Reward.h"

namespace Game
{
	/*
	 
	 Power ups
	 
	 - Some power ups affect weapon states, powerup state is tracked in weapon slots
	 - Some power ups have a different effect, these require special handling
	 
	 STRENGTH (RED)
	 - Maximum damage powerup. Weapons become extremely powerful for a limited amount of time
	 - Rapid fire. Firing rate slashes in 2 or 3 for a limited amount of time
	 
	 MONEY (GRAY + GOLD)
	 - Credits drop.
	 
	 SHIELD (BLUE)
	 - Shield powerup. Acts like an overshield.
	 - Invincible (collision damages enemies) (?)
	 
	 FUN (GREEN)
	 - Disorient enemies (?)
	 - Slow down enemies (?)
	 - Beam fever (beams emanating uncontrollably)
	 - Rain (starts a nice rain drops effect)
	 - Paddo (inverts controls or whatever..)
	 
	 */
	
	class PlayerShield
	{
	public:
		PlayerShield();
		void Initialize();
		
		void Update(float dt);
		void Render(const Vec2F& pos);
		
		Gauge m_Capacity;
	};
	
	typedef float (CbEvaluateMissileTarget)(const Vec2F& pos, const Vec2F& dir, const Entity* target);
	
	class EntityPlayer : public Entity
	{
	public:
		EntityPlayer();
		~EntityPlayer();
		void Initialize();
		
		void Setup(Vec2F pos);
		
		virtual void Update(float dt);
	private:
		void UpdateCollision();
		void UpdateControl(float dt);
		void UpdateMovement(float dt);
		void WrapPosition(float dt);
		void UpdateFiring(float dt);
		void DoFire(float dt);
		void UpdateShield(float dt);
		void UpdateTempPowerups(float dt);
	public:
		virtual void UpdateSB(SelectionBuffer* sb);
		virtual void Render();
		virtual void Render_Additive();
		void Render_SpawnAnimation();
		void Render_HealthRing();
		void Render_Beams();
		void Render_Spray();
		void Render_HUD();
		
		void SpawnDamageParticles(const Vec2F& pos);
		virtual void HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, DamageType type);
		virtual void HandleHit(const Vec2F& pos, Entity* hitEntity);
		
		void HandlePowerup(PowerupType type);
		void HandlePowerball(PowerballType type);
		void HandleKill(EntityClass type);
		void HandleReward(const Reward& reward);
		
		XBOOL Intersect_LineSegment(const Vec2F& p, const Vec2F& d, Vec2F& out_HitLocation) const;
		inline bool IsPlaying_get() const
		{
			return m_State == State_Playing;
		}

		CallBack OnDeath;
		CallBack OnGameOver;
		
	private:
		enum State
		{
			State_Spawning,
			State_Playing,
			State_Dying,
			State_Dead
		};
		
		// --------------------
		// Logic
		// --------------------
		void State_set(State state);
		
		State m_State;
		PolledTimer m_DyingTimer;
		PolledTimer m_DyingExplodeTimer;
		InvincibilityTimer m_InvincibilityTimer;
		
		// --------------------
		// Spawning
		// --------------------
		Vec2F GetSpawnPosition();
		void SpawnSpawnParticles(Vec2F pos);
		
		PolledTimer m_SpawnWaitTimer;
		PolledTimer m_SpawningTimer;
		AnimTimer m_SpawningAnimTimer;
		
		// --------------------
		// Lives
		// --------------------
		void IncreaseLives();
		void DecreaseLives();
		
		int m_Lives;
		
	public:
		inline int Lives_get() const
		{
			return m_Lives;
		}
		inline void Lives_set(int lives)
		{
			m_Lives = lives;
		}
		
		// --------------------
		// Credits
		// --------------------
	public:
		int Credits_get();
		void Credits_Increase(int amount);
		void Credits_Decrease(int amount);
		
	private:
		int m_Credits;
		
	public:
		// --------------------
		// Upgrades
		// --------------------
		void HandleUpgrade(UpgradeType upgrade);
		int GetUpgradeLevel(UpgradeType upgrade);
		int GetMaxUpgradelevel(UpgradeType upgrade);
		void SetAllUpgradeLevel(bool t);
		
	private:
		// --------------------
		// Powerups
		// --------------------
		void Powerups_HandleDeath();
		void DisableAllPowerups();
	public:
		void RemoveTempPowerups();
		bool HasPaddoPowerup();
		bool HasSlowMoPowerup();
		float GetSlowMoTimeDilution();

	private:
		bool m_HasPowerup_EnergyMax;
		bool m_HasPowerup_RapidFire;
		bool m_HasPowerup_ShieldMax;
		bool m_HasPowerup_DisorientEnemies;
		TriggerTimerW m_HasPowerup_BeamFever;
		bool m_HasPowerup_Rain;
//		bool m_HasPowerup_Paddo; // done
		TriggerTimerW m_HasPowerup_Paddo;
		TriggerTimerG m_HasPowerup_SlowMo;

		// --------------------
		// Weapons
		// --------------------
	public:
		void SwitchWeapons();
		WeaponType CurrentWeapon_get() const;
		
	private:
		WeaponSlot m_WeaponSlots[WeaponType_ZCOUNT];
		LaserBeamMgr m_LaserBeamMgr;
		WeaponType m_CurrentWeapon;
		
		// --------------------
		// Targeting
		// --------------------
		float GetTargetingAngle_Random() const;
		void GetTargetingAngles_Random(int angleCount, float* out_Angles) const;
		void GetTargetingAngles_Even(int angleCount, float* out_Angles) const;
		
		float m_TargetingCone_BaseAngle;
		float m_TargetingCone_SpreadAngle;
		
		// --------------------
		// Firing
		// --------------------
		CD_TYPE SelectMissileTarget(const Vec2F& pos, const Vec2F& dir);
		static CD_TYPE SelectMissileTarget_Static(class Bullet* bullet, void* obj);
		float GetMissileHuntTime();
		
		PolledTimer m_FiringTimer;
		bool m_FiringIsActive;
		
		// --------------------
		// Shock attack
		// --------------------
		void UpdateShock(float dt);
		void Shock_Begin();
		void Shock_HandleDeath();
		static void HandleShock(void* obj, void* arg);
	public:
		float ShockTimeDilution_get() const;
		int ShockLevel_get() const;
	private:
		int m_ShockLevel;
		TimeTracker m_TimeTrackerPlayer;
		TriggerTimer m_ShockSlowdownTimer;
		TriggerTimer m_ShockCooldownTimer;
		Anim_ScreenFlash m_ShockFlash;
		
		// --------------------
		// Special attack
		// --------------------
	public:
		void SpecialAttack_Begin();
		float SpecialAttackFill_get() const;
		
	private:
		bool m_SpecialIsActive;
		Gauge m_SpecialAttack;
		int m_SpecialMultiplier;
		PolledTimer m_MissileTimer;
		
		// --------------------
		// Shield
		// --------------------
		Gauge m_ShieldStrength;
		float m_ShieldRestorePerSecond;
		PolledTimer m_ShieldRestoreTimer;
		Fader m_ShieldFader;
		
		// --------------------
		// Overshield
		// --------------------
		Gauge m_OverShieldStrength;
		PolledTimer m_OverShieldCrossTimer;
		
		// --------------------
		// Border patrol
		// --------------------
		int m_PatrolEscalation;

		// --------------------
		// Statistics
		// --------------------
	public:
		int m_Stat_KillCount;
		int m_Stat_BulletCount;
	private:
		
		// --------------------
		// Cheats
		// --------------------
	public:
		void Cheat_Invincibility();
		void Cheat_PowerUp();
		
	private:
		bool m_IsInvincible;
		
		// --------------------
		// Animation
		// --------------------
		AnimTimer m_AnimHit;
		PolledTimer m_ExplosionTimer;
		AnimTimer m_HealthCircleFadeTimer;
		float m_DamageParticleAngle;
		
		// --------------------
		// Load & Save
		// --------------------
	public:
		void Load(Archive& a);
		void Save(Archive& a);
		void SaveUpdate(Archive& a);
	private:
		
		LogCtx m_Log;
		
		friend class WeaponSlot;
	};
}
