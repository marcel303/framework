#include <cmath>
#include "Bandit.h"
#include "BanditEntity.h"
#include "EntityPlayer.h"
#include "GameRound.h"
#include "GameState.h"
#include "ParticleGenerator.h"
#include "Path.h"
#include "RandomPicker.h"
#include "ResIO.h"
#include "SoundEffectMgr.h"
#include "Stream.h"
#include "TempRender.h"
#include "Textures.h"
#include "UsgResources.h"
#include "Util_ColorEx.h"
#include "VectorShape.h"
#include "World.h"

#define BEAM_LENGTH 1500.0f

#define PANIC_INTERVAL (1.0f + Calc::Random(2.0f))
#define PANIC_FIRE_COUNT 40
#define PANIC_FIRE_RATE 20.0f
#define PANIC_FIRE_INTERVAL (1.0f / PANIC_FIRE_RATE)

#define PURPLE_TARGET_RANGE 250.0f

#define VULCAN_SPEED 150.0f
#define VULCAN_DAMAGE 3.0f
#define VULCAN_OFFSET 20.0f
#define VULCAN_SIZE 25.0f
#define VULCAN_TANGENT 8.0f

static inline float DAMAGE_MULTIPLIER_INSTANT()
{
    if (g_GameState->m_GameRound->GameModeIsClassic())
        return Calc::Max(0.2f, 1.0f - 0.025f * g_GameState->m_GameRound->Classic_Level_get());
    else
        return 1.0f;
}

static inline float DAMAGE_MULTIPLIER_OVERTIME()
{
    if (g_GameState->m_GameRound->GameModeIsClassic())
        return Calc::Max(0.1f, 1.0f - 0.05f * g_GameState->m_GameRound->Classic_Level_get());
    else
        return 1.0f;
}

#define UseFixedAngles() (g_GameState->ActiveView_get() == View_BanditIntro)

namespace Bandits
{
	Link::Link() : Game::Entity()
	{
		Initialize();
	}
	
	Link::~Link()
	{
		Allocate(0);
	}
	
	void Link::Initialize()
	{
		Entity::Initialize();
		
		// entity overrides
		
		Class_set(Game::EntityClass_MaxiBossSegment);
		
		// link
		
		m_Parent = 0;
		m_Links = 0;
		m_LinkCount = 0;
		m_InsulationFactor = 0;
		m_Shape = 0;
		m_MirrorX = false;
		m_MirrorY = false;
		m_LinkFlags = 0;
		m_LinkType = LinkType_Undefined;
		m_LocalPosition = Vec2F(0.0f, 0.0f);
		m_GlobalRotation = 0.0f;
		m_Weapon = Weapon_Undefined;
		m_WeaponTarget = false;
		m_BeamActivationCount = 0;
		
		m_HitTimer.Initialize(g_GameState->m_TimeTracker_World, false);
	}
	
	void Link::Setup(Bandit* bandit, Link* parent, Vec2F pos, float baseAngle, float angle, float minAngle, float maxAngle, float angleSpeed, VectorShape* shape, bool mirrorX, bool mirrorY, int flags)
	{
		// entity
		
		Position_set(pos);
		Rotation_set(baseAngle + angle);
		IgnoreId_set(bandit);
		
		// bandit
		
		m_Bandit = bandit;
		m_Parent = parent;
		m_Shape = shape;
		m_MirrorX = mirrorX;
		m_MirrorY = mirrorY;
		m_LinkFlags = flags;
		m_LocalPosition = pos;
		m_BaseAngle = baseAngle;
		m_MinAngle = minAngle;
		m_MaxAngle = maxAngle;
		m_VarAngle = angle;
		m_AngleSpeed = angleSpeed;
		m_AngleController.Setup(0.0f, 0.0f, 0.0f);
		m_Weapon_AngleController.Setup(0.0f, 0.0f, 1.8f);
		m_VulcanPattern = VulcanPattern_Targeted;
		m_VulcanPattern_Angle = 0.0f;
		
		bool isWeapon = false;
		
		//
		
		if (flags & VRCC::CompositionFlag_Weapon_Vulcan)
		{
			m_Shape = g_GameState->GetShape(Resources::BANDIT_POD_VULCAN);
			m_Weapon = Weapon_Vulcan;
			m_WeaponTarget = true;
			isWeapon = true;
		}
		if (flags & VRCC::CompositionFlag_Weapon_Missile)
		{
			m_Shape = g_GameState->GetShape(Resources::BANDIT_POD_MISSILE);
			m_Weapon = Weapon_Missile;
			m_WeaponTarget = false;
			isWeapon = true;
		}
		if (flags & VRCC::CompositionFlag_Weapon_Beam)
		{
			m_Shape = g_GameState->GetShape(Resources::BANDIT_POD_BEAM);
			m_Weapon = Weapon_Beam;
			m_WeaponTarget = false;
			isWeapon = true;
		}
		if (flags & VRCC::CompositionFlag_Weapon_BlueSpray)
		{
			m_Shape = g_GameState->GetShape(Resources::BANDIT_POD_BLUE);
			m_Weapon = Weapon_BlueSpray;
			m_WeaponTarget = false;
			isWeapon = true;
		}
		if (flags & VRCC::CompositionFlag_Weapon_PurpleSpray)
		{
			m_Shape = g_GameState->GetShape(Resources::BANDIT_POD_PURPLE);
			m_Weapon = Weapon_PurpleSpray;
			m_WeaponTarget = true;
			isWeapon = true;
		}
		
		if (isWeapon)
			m_LinkType = LinkType_Weapon;
		else if (parent)
			m_LinkType = LinkType_Segment;
		else
			m_LinkType = LinkType_Core;

		switch (m_LinkType)
		{
			case LinkType_Core:
				HitPoints_set(12.0f);
				break;
			case LinkType_Segment:
				HitPoints_set(6.0f);
				break;
			case LinkType_Weapon:
				HitPoints_set(2.0f);
				break;
				
			default:
#ifndef DEPLOYMENT
				throw ExceptionVA("not implemented");
#else
				break;
#endif
		}
		
		if (m_LinkType == LinkType_Weapon && m_Weapon != Weapon_Beam)
		{
			m_AttackTrigger.Start(5.0f + Calc::Random(10.0f));
		}
		
		Assert(m_Shape != 0);
	}
	
	void Link::Finalize()
	{
		if (IsAlive_get())
		{
			// provide insulation to uptream links
			
			if (m_LinkType != LinkType_Weapon && !(m_LinkFlags & VRCC::CompositionFlag_Defense_Armour))
			{
				InsulationTraverseUp(+1);
			}
			
			m_InsulationFactor = 1;
			
			for (int i = 0; i < m_LinkCount; ++i)
			{
				m_Links[i]->Finalize();
			}
			
			m_AngleController.Setup(0.0f, 0.0f, m_Bandit->m_Entity->LinkRotationSpeedFactor_get());
		}
		else
		{
			IsDeadTraverseDown();
		}
	}
	
	void Link::Update(float dt)
	{
		Entity::Update(dt);
		
		UpdateRotation(dt);
		UpdateTransform();
		UpdateWeapon(dt);
		UpdatePanicAttack(dt);
		
		// did we die?
		
		if (Flag_IsSet(Game::EntityFlag_DidDie))
		{
			HandleDie();
		}
	}
	
	void Link::UpdateSB(SelectionBuffer* sb)
	{	
		if (!m_Shape)
			return;
		
		float scaleX = 1.0f;
		float scaleY = 1.0f;
		
		if (m_MirrorX)
			scaleX = -1.0f;
		if (m_MirrorY)
			scaleY = -1.0f;
		
		switch (m_LinkType)
		{
			case LinkType_Core:
			case LinkType_Segment:
				if (m_Shape)
					g_GameState->UpdateSBWithScale(m_Shape, m_GlobalPosition[0], m_GlobalPosition[1], m_GlobalRotation, SelectionId_get(), scaleX, scaleY);
				break;
			case LinkType_Weapon:
				g_GameState->UpdateSBWithScale(m_Shape, m_GlobalPosition[0], m_GlobalPosition[1], WeaponAngle_get(), SelectionId_get(), scaleX, scaleY);
				break;
			case LinkType_Undefined:
				break;
		}
	}
	
	int Link::ActivateBeamAttack()
	{
		if (!IsAlive_get())
			return 0;
		
		int result = 0;
		
		if (m_LinkType == LinkType_Weapon && m_Weapon == Weapon_Beam)
		{
			Game::MaxiLaserBeam* beam = m_Bandit->AllocateBeam();
			
			beam->Start(100.0f, BEAM_LENGTH, 5.0f, this);
			
			result++;
		}
		
		// activate beam attack on children
		
		for (int i = 0; i < m_LinkCount; ++i)
			result += m_Links[i]->ActivateBeamAttack();
		
		return result;
	}
		
	Link* Link::Child_get(int index)
	{
		return m_Links[index];
	}
	
	void Link::Child_set(int index, Link* link)
	{
		delete m_Links[index];
		m_Links[index] = 0;
		
		m_Links[index] = link;
	}
	
	void Link::ChildCount_set(int childCount)
	{
		Allocate(childCount);
	}
	
	LinkType Link::LinkType_get() const
	{
		return m_LinkType;
	}
	
	Weapon Link::WeaponType_get() const
	{
		return m_Weapon;
	}
	
	// --------------------
	// Transform
	// --------------------
	
	void Link::UpdateRotation(float dt)
	{
		if (!m_Parent)
		{
			// root link. rotation controlled by bandit entity
			return;
		}

		if (UseFixedAngles())
		{
			Rotation_set(m_BaseAngle + m_VarAngle);
			return;
		}
		
		if (m_LinkFlags & VRCC::CompositionFlag_Behaviour_AngleFollow)
		{
			if (m_Bandit->m_Entity->IsBeamActive_get())
				return;
			
			// update rotation angle based on behaviour
			
			Vec2F delta = Game::g_World->m_Player->Position_get() - m_GlobalPosition;
			
			float targetAngle = Vec2F::ToAngle(delta) - (m_Parent->m_GlobalRotation + m_BaseAngle);
			
			m_AngleController.TargetAngle_set(targetAngle);
			m_AngleController.Update(dt);
			
			// get angle & clamp
			
			m_VarAngle = m_AngleController.Angle_get();
			
			if (m_VarAngle < m_MinAngle)
				m_VarAngle = m_MinAngle;
			if (m_VarAngle > m_MaxAngle)
				m_VarAngle = m_MaxAngle;
			
			m_AngleController.Angle_set(m_VarAngle);
			
			// update rotation
			
			Rotation_set(m_BaseAngle + m_VarAngle);
		}
		else if (m_LinkFlags & VRCC::CompositionFlag_Behaviour_AngleSpin)
		{
			Rotation_set(Rotation_get() + m_AngleSpeed * dt);
		}
	}
	
	void Link::UpdateTransform()
	{
		// calculate 2x3 transformation matrix
		
		Mat3x2 mat;
		
		// fixme: why is a negative rotation required here?
		mat.MakeTransform(m_LocalPosition, -Rotation_get());
		
		// make it global by multiplying with parent's global transform
		
		if (m_Parent)
		{
			mat = m_Parent->m_GlobalTransform * mat;
		}
		
		m_GlobalTransform = mat;
		
		m_GlobalPosition = m_GlobalTransform * Vec2F(0.0f, 0.0f);
		
		Position_set(m_GlobalPosition);
		
		// update cumulative rotation (cheaper than transforming a unit vector w/ matrix and calculating the angle from that)
		
		float r = Rotation_get();
		
		if (m_Parent)
			r += m_Parent->m_GlobalRotation;

		m_GlobalRotation = r;
	}
	
	// --------------------
	// Weapons
	// --------------------
	
	void Link::UpdateWeapon(float dt)
	{
		if (m_LinkType != LinkType_Weapon)
			return;
		
		// update turret rotation
		
		if (m_WeaponTarget && !UseFixedAngles())
		{
			Vec2F delta = Game::g_World->m_Player->Position_get() - m_GlobalPosition;
			
			float targetAngle = Vec2F::ToAngle(delta) - m_GlobalRotation;
			
			m_Weapon_AngleController.TargetAngle_set(targetAngle);
			
			m_Weapon_AngleController.Update(dt);
		}
		
		// update attack

		if (m_AttackTrigger.Read())
		{
			m_AttackLoadTrigger.Start(0.3f);
		}

		if (m_AttackLoadTrigger.Read())
		{
			// begin new attack
			
			switch (m_Weapon)
			{
				case Weapon_Vulcan:
				{
					float interval = (VULCAN_SIZE + 5.0f) / VULCAN_SPEED;
					m_FireTimer.Start(GetVulcanRepeatCount(), interval);
					m_VulcanPattern = VulcanPattern_Targeted;
					break;
				}
				case Weapon_Missile:
				{
					m_FireTimer.Start(1, 0.0f);
					break;
				}
				case Weapon_BlueSpray:
				{
					int n = 20;
					m_FireTimer.Start(n, 0.5f / n);
					g_GameState->m_SoundEffects->Play(Resources::SOUND_MAXI_BLUE, 0);
					break;
				}
				case Weapon_PurpleSpray:
				{
					int n = 40;
					m_FireTimer.Start(n, 1.0f / 20.0f);
					g_GameState->m_SoundEffects->Play(Resources::SOUND_MAXI_PURPLE, 0);
					break;
				}
					
				case Weapon_Beam:
				case Weapon_Undefined:
					break;
			}
			
			m_AttackTrigger.Start(4.0f + Calc::Random(4.0f));
		}
		
		// update firing
		
		while (m_FireTimer.ReadTick())
		{
			HandleFire();
		}
	}
	
	float Link::WeaponAngle_get() const
	{
		return m_Weapon_AngleController.Angle_get() + m_GlobalRotation;
	}
	
	bool Link::BeamEnabled_get() const
	{
		return true;
	}
	
	Vec2F Link::WeaponTargetDir_get() const
	{
		return Vec2F::FromAngle(WeaponAngle_get());
	}
	
	static CD_TYPE MissileTargetSelect(Game::Bullet* bullet, void* obj)
	{
		return Game::g_World->m_Player->SelectionId_get();
	}
	
	void Link::HandleFire()
	{
#ifndef DEPLOYMENT
#pragma message("bandit fire hack")
		if (g_GameState->m_GameRound->Modifier_MakeLoveNotWar_get())
		{
			float angle = WeaponAngle_get();
			Vec2F dir = m_Bandit->m_Entity->Speed_get() + Vec2F::FromAngle(angle);
			
			Particle& p = g_GameState->m_ParticleEffect.Allocate(g_GameState->GetTexture(Textures::PARTICLE_HEART)->m_Info, 0, 0);
			
			Particle_RotMove_Setup(
			   &p,
			   GlobalPosition_get()[0],
			   GlobalPosition_get()[1], 1.0f, 20.0f, 20.0f, 0.0f, dir[0] * 60.0f, dir[1] * 60.0f, 0.0f);
			
			p.m_Color = SpriteColors::White;
			
			return;
		}
#endif
		
		const float blueSpeed = 200.0f;
		const float purpleSpeed = 70.0f;
		
		if (m_Weapon == Weapon_Vulcan)
		{
			FireVulcan(m_VulcanPattern);
		}
		if (m_Weapon == Weapon_Missile)
		{
			Game::Bullet bullet;
			bullet.MakeMissile(IgnoreId_get(), GlobalPosition_get(), Vec2F::FromAngle(WeaponAngle_get()), MissileTargetSelect, 0, m_Bandit->m_Entity->MissileHuntTime_get());
			Game::g_World->SpawnBullet(bullet);
		}
		if (m_Weapon == Weapon_BlueSpray)
		{
			const Vec2F speed = Vec2F::FromAngle(Calc::Random(Calc::m2PI)) * blueSpeed;
			Game::Bullet bullet;
			bullet.MakeMaxiBlueSpray(IgnoreId_get(), GlobalPosition_get(), speed, 2.0f);
			Game::g_World->SpawnBullet(bullet);
		}
		if (m_Weapon == Weapon_PurpleSpray)
		{
			const float distanceSq = (m_GlobalPosition - Game::g_Target).LengthSq_get();
			const float angle = distanceSq > PURPLE_TARGET_RANGE * PURPLE_TARGET_RANGE ? m_GlobalRotation : WeaponAngle_get();
			const float angleDelta = Calc::Random_Scaled(Calc::DegToRad(15.0f));
			const Vec2F speed = Vec2F::FromAngle(angle + angleDelta) * purpleSpeed;
			Game::Bullet bullet;
			bullet.MakeMaxiPurpleSpray(IgnoreId_get(), GlobalPosition_get(), speed, 3.0f);
			Game::g_World->SpawnBullet(bullet);
		}
	}
	
	void Link::FireVulcan(VulcanPattern pattern)
	{
		switch (pattern)
		{
			case VulcanPattern_Targeted:
			{
				const Vec2F dir = WeaponTargetDir_get();
				const Vec2F tangent(-dir[1], dir[0]);
				
				Game::Bullet bullet;
				
				bullet.MakeVulcan(IgnoreId_get(), m_GlobalPosition + dir * VULCAN_OFFSET - tangent * VULCAN_TANGENT * 0.5f, dir * VULCAN_SPEED, Game::VulcanType_Boss, VULCAN_DAMAGE);
				Game::g_World->SpawnBullet(bullet);
				
				bullet.MakeVulcan(IgnoreId_get(), m_GlobalPosition + dir * VULCAN_OFFSET + tangent * VULCAN_TANGENT * 0.5f, dir * VULCAN_SPEED, Game::VulcanType_Boss, VULCAN_DAMAGE);
				Game::g_World->SpawnBullet(bullet);
				
				break;
			}
			
			case VulcanPattern_CirclePulse:
			{
				for (int i = 0; i < 12; ++i)
				{
					const Vec2F dir = Vec2F::FromAngle(m_VulcanPattern_Angle);
					
					m_VulcanPattern_Angle += Calc::m2PI / 12.0f;
						
					Game::Bullet bullet;
					bullet.MakeVulcan(IgnoreId_get(), m_GlobalPosition + dir * VULCAN_OFFSET, dir * VULCAN_SPEED, Game::VulcanType_Boss, VULCAN_DAMAGE);
					Game::g_World->SpawnBullet(bullet);
				}
				
				break;
			}
			
			case VulcanPattern_CircleRotate:
			{
				const Vec2F dir = Vec2F::FromAngle(m_VulcanPattern_Angle);
				
				m_VulcanPattern_Angle += Calc::m2PI / 10.0f;
				
				Game::Bullet bullet;
				bullet.MakeVulcan(IgnoreId_get(), m_GlobalPosition + dir * VULCAN_OFFSET, dir * VULCAN_SPEED, Game::VulcanType_Boss, VULCAN_DAMAGE);
				Game::g_World->SpawnBullet(bullet);
				
				break;
			}
			
			case VulcanPattern__Count:
#ifndef DEPLOYMENT
				throw ExceptionNA();
#else
				break;
#endif
		}
	}
	
	int Link::GetVulcanRepeatCount() const
	{
		int max = m_Bandit->m_Entity->Level_get() / 50;
		
		int min = max / 2;
		
		if (max == 0)
			max = 1;
		if (min == 0)
			min = 1;
		
		return Calc::Random(min, max);
	}
	
	// --------------------
	// Panic attack
	// --------------------
	void Link::UpdatePanicAttack(float dt)
	{
		if (m_PanicAttackTrigger.Read())
		{
			Game::RandomPicker<VulcanPattern, VulcanPattern__Count> picker;
			
			picker.Add(VulcanPattern_CircleRotate, 2.0f);
			picker.Add(VulcanPattern_CirclePulse, 1.0f);
			
			m_VulcanPattern = picker.Get();
			
			switch (m_VulcanPattern)
			{
				case VulcanPattern_CircleRotate:
					m_PanicFireTimer.Start(PANIC_FIRE_COUNT, PANIC_FIRE_INTERVAL);
					break;
				case VulcanPattern_CirclePulse:
					m_PanicFireTimer.Start(1, 2.0f);
					break;
				default:
					break;
			}
		}
		
		while (m_PanicFireTimer.ReadTick())
		{
			FireVulcan(m_VulcanPattern);
			
			if (m_PanicFireTimer.IsEmpty_get())
			{
				m_PanicAttackTrigger.Start(PANIC_INTERVAL);
			}
		}
	}
	
	void Link::BeginPanicAttack()
	{
		m_PanicAttackTrigger.Start(0.0f);
	}
	
	// --------------------
	// Drawing
	// --------------------
	
	void Link::Render_Below()
	{
		// render
		
		if (m_LinkFlags & VRCC::CompositionFlag_Animation_Thruster)
		{
			if (m_Bandit->m_Entity->IsThrusterEnabled_get())
			{
				SpriteColor color = Calc::Color_FromHue(g_GameState->m_GameRound->BackgroundHue_get());
//				color.v[3] = Calc::Random(191, 255);
//				g_GameState->Render(g_GameState->GetShape(Resources::BANDIT_THRUSTER), m_GlobalPosition, m_Bandit->m_Entity->ThrusterAngle_get(), SpriteColors::Black);
				g_GameState->Render(g_GameState->GetShape(Resources::BANDIT_THRUSTER), m_GlobalPosition, m_Bandit->m_Entity->ThrusterAngle_get(), color);
			}
		}
	}
	
	void Link::Render()
	{
		DoRender();
	}
		
	void Link::DoRender()
	{
		m_HitTimer.Tick();
		
		SpriteColor baseColor = SpriteColors::Black;
		
		if (m_LinkFlags & VRCC::CompositionFlag_Defense_Shield)
			baseColor = SpriteColor_Make(0, 127, 255, 255);
		if (m_LinkFlags & VRCC::CompositionFlag_Defense_Armour)
			baseColor = SpriteColor_Make(255, 227, 0, 255);
		
		SpriteColor color = SpriteColor_BlendF(baseColor, SpriteColors::HitEffect_Boss, m_HitTimer.Progress_get());
		
		float scaleX = 1.0f;
		float scaleY = 1.0f;
		
		if (m_MirrorX)
			scaleX = -1.0f;
		if (m_MirrorY)
			scaleY = -1.0f;
		
		switch (m_LinkType)
		{
			case LinkType_Core:
				{
#ifndef DEPLOYMENT
#pragma message("hearts hack enabled")
					const VectorShape* shape =
						g_GameState->m_GameRound->Modifier_MakeLoveNotWar_get() ?
							g_GameState->GetShape(Resources::BANDIT_CORE_FRIENDLY) :
							m_Shape;
					
					const float rotation = g_GameState->m_GameRound->Modifier_MakeLoveNotWar_get() ? 0.0f :
					m_GlobalRotation;
#else
					const VectorShape* shape = m_Shape;
					const float rotation = m_GlobalRotation;
#endif
					
					if (shape)
					{
						// draw shape using global position / rotation
						
						g_GameState->RenderWithScale(shape, m_GlobalPosition, rotation, color, scaleX, scaleY);
					}
				}
				break;
			case LinkType_Segment:
				if (m_Shape)
				{
					// draw shape using global position / rotation
					
					g_GameState->RenderWithScale(m_Shape, m_GlobalPosition, m_GlobalRotation, color, scaleX, scaleY);
				}
				break;
			case LinkType_Weapon:
				g_GameState->Render(m_Shape, m_GlobalPosition, WeaponAngle_get(), color);
				
				if (m_AttackLoadTrigger.IsRunning_get())
				{
					const int c = (int)((1.0f - m_AttackLoadTrigger.Progress_get()) * 255.0f);
					g_GameState->Render(m_Shape, m_GlobalPosition, WeaponAngle_get(), SpriteColor_Make(c, c, c, 255));
				}
				break;
			case LinkType_Undefined:
				break;
		}
	}
	
	void Link::HandleDamage(const Vec2F& pos, const Vec2F& impactSpeed, float damage, Game::DamageType type)
	{
		if (m_LinkType == LinkType_Weapon && type == Game::DamageType_OverTime)
			damage *= 0.5f;
		
		if (m_LinkType == LinkType_Core && m_InsulationFactor > 1)
			damage /= 10.0f;
		
		if (m_LinkFlags & VRCC::CompositionFlag_Defense_Shield)
			damage /= 10.0f;
		if (m_LinkFlags & VRCC::CompositionFlag_Defense_Armour)
			damage = 0.0f;
		
		damage = damage / std::pow(1.5f, m_InsulationFactor);
		
		if (type == Game::DamageType_OverTime)
			damage *= DAMAGE_MULTIPLIER_OVERTIME();
		if (type == Game::DamageType_Instant)
			damage *= DAMAGE_MULTIPLIER_INSTANT();
		
		Entity::HandleDamage(pos, impactSpeed, damage, type);	
		
		m_HitTimer.Start(AnimTimerMode_FrameBased, true, 10.0f, AnimTimerRepeat_None);
	}
	
	void Link::HandleDamage_Begin(const Vec2F& pos, const Vec2F& impactSpeed, float damage, Game::DamageType type)
	{
		switch (m_LinkType)
		{
			case LinkType_Core:
				g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_CORE_HIT, 0);
				break;
			case LinkType_Segment:
				g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_SEGMENT_HIT, 0);
				break;
			case LinkType_Weapon:
				g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_SEGMENT_HIT, 0);
				break;
			case LinkType_Undefined:
				break;
		}
	}
	
	void Link::HandleDie()
	{
		DoDestroy();
		
		// tell parent we died
		
		if (m_Parent)
		{
			m_Parent->HandleDeath(this);
		}
		
		m_Bandit->m_Entity->HandleDeath(this);
	}
	
	void Link::HandleDeath(Link* link)
	{
	}
	
	void Link::DoDestroy()
	{
		// destroy child links
		
		for (int i = 0; i < m_LinkCount; ++i)
		{
			if (!m_Links[i]->IsAlive_get())
				continue;
			
			m_Links[i]->DoDestroy();
		}
		
		// we're dead
		
		IsAlive_set(XFALSE);
		
		// remove insulation
		
		if (m_LinkType != LinkType_Weapon)
		{
			// recalculate insulation factor
			
			InsulationTraverseUp(-1);
		}
		
		// spawn explosion particles
		
		ParticleGenerator::GenerateRandomExplosion(g_GameState->m_ParticleEffect, m_GlobalPosition, 200.0f, 250.0f, 0.8f, 15, g_GameState->GetTexture(Textures::EXPLOSION_LINE)->m_Info, 1.0f, 0);
		
		// all hell breaks loose as we play this sound~
		
		switch (m_LinkType)
		{
			case LinkType_Core:
				g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_CORE_DESTROY, 0);
				break;
			case LinkType_Segment:
				g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_SEGMENT_DESTROY, 0);
				break;
			case LinkType_Weapon:
				g_GameState->m_SoundEffects->Play(Resources::SOUND_BANDIT_SEGMENT_DESTROY, 0);
				break;
			case LinkType_Undefined:
				break;
		}
		
		// flaming balls of fire
		
#if 0 // fixme, create a new sprite anim
		Mat3x2 mat;
		
		mat.MakeRotation(m_GlobalRotation);
		
		for (int i = 0; i < 4; ++i)
		{
			Vec2F offset(Calc::Random_Scaled(40.0f), Calc::Random_Scaled(40.0f));
			Vec2F pos = m_GlobalPosition + offset;
			Vec2F explosionSize(64.0f, 64.0f);
			Game::SpriteAnim anim;
			anim.Setup(g_GameState->m_ResMgr.Get(Resources::EXPLOSION_01), 4, 4, 0, 15, 24, &g_GameState->m_TimeTracker_World);
			anim.Start();
			g_GameState->m_SpriteEffectMgr.Add(anim, pos - explosionSize / 2.0f, explosionSize, SpriteColors::Black);
		}
#endif
	}
	
	void Link::Allocate(int childCount)
	{
		for (int i = 0; i < m_LinkCount; ++i)
		{
			delete m_Links[i];
			m_Links[i] = 0;
		}
		delete[] m_Links;
		m_LinkCount = 0;
		m_Links = 0;
		
		if (childCount > 0)
		{
			m_Links = new Link*[childCount];
			for (int i = 0; i < childCount; ++i)
				m_Links[i] = 0;
			m_LinkCount = childCount;
		}
	}
	
	void Link::InsulationTraverseUp(int delta)
	{
		for (Link* parent = m_Parent; parent && parent->IsAlive_get(); parent = parent->m_Parent)
		{
			parent->m_InsulationFactor += delta;
			
			//LOG(LogLevel_Debug, "insulation (%d): %d", (int)parent->SelectionId_get(), parent->m_InsulationFactor);
		}
	}
	
	void Link::IsDeadTraverseDown()
	{
		for (int i = 0; i < m_LinkCount; ++i)
		{
			m_Links[i]->IsAlive_set(XFALSE);
			m_Links[i]->IsDeadTraverseDown();
		}
	}
	
	EntityBandit* Link::Entity_get()
	{
		return m_Bandit->m_Entity;
	}
	
	// ====================
	// Bandit
	// ====================
	
	Bandit::Bandit()
	{
		Initialize();
	}
	
	Bandit::~Bandit()
	{
		StopBeamAttack();
		
		delete m_RootLink;
		m_RootLink = 0;
		
		delete[] m_Links;
		m_Links = 0;
	}
	
	void Bandit::Initialize()
	{
		m_Entity = 0;
		m_RootLink = 0;
		m_Links = 0;
		m_LinkCount = 0;
	}
	
	void Bandit::Setup(EntityBandit* entity, Link** links, int linkCount, Link* rootLink)
	{
		m_Entity = entity;
		m_RootLink = rootLink;
		m_Links = links;
		m_LinkCount = linkCount;
	}
	
	void Bandit::Finalize()
	{
		if (m_RootLink)
			m_RootLink->Finalize();
	}
	
	void Bandit::Update(float dt)
	{
		for (int i = 0; i < m_LinkCount; ++i)
		{
			if (!m_Links[i]->IsAlive_get())
				continue;
			
			m_Links[i]->Update(dt);
		}
		
		m_LaserBeamMgr.Update(dt);
	}
	
	void Bandit::UpdateSB(SelectionBuffer* sb)
	{
		for (int i = 0; i < m_LinkCount; ++i)
		{
			if (!m_Links[i]->IsAlive_get())
				continue;
			
			m_Links[i]->UpdateSB(sb);
		}
	}
	
	void Bandit::Render_Below()
	{
		for (int i = 0; i < m_LinkCount; ++i)
		{
			if (!m_Links[i]->IsAlive_get())
				continue;
			
			m_Links[i]->Render_Below();
		}
	}
	
	void Bandit::Render()
	{
		if (g_GameState->m_DrawMode == VectorShape::DrawMode_Silhouette)
			return;
		
		for (int i = 0; i < m_LinkCount; ++i)
		{
			if (!m_Links[i]->IsAlive_get())
				continue;
			
			m_Links[i]->Render();
		}
	}
	
	// --------------------
	// Links
	// --------------------
	
	Link* Bandit::RootLink_get()
	{
		return m_RootLink;
	}
	
	Link** Bandit::LinkArray_get()
	{
		return m_Links;
	}
	
	int Bandit::LinkArraySize_get()
	{
		return m_LinkCount;
	}
	
	// --------------------
	// Laser beam attack
	// --------------------
	
	Game::MaxiLaserBeam* Bandit::AllocateBeam()
	{
		return m_LaserBeamMgr.Allocate();
	}
	
	void Bandit::StopBeamAttack()
	{
		m_LaserBeamMgr.Clear();
	}
	
	bool Bandit::StartBeamAttack()
	{
		return m_RootLink->ActivateBeamAttack() != 0;
	}
}
