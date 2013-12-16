#include "BanditEntity.h"
#include "BanditIO.h"
#include "EntityPlayer.h"
#include "GameState.h"
#include "SoundEffect.h"
#include "UsgResources.h"
#include "World.h"

#define ENTER_DISTANCE 300.0f
#define ENTER_SPEED 100.0f
#define MOVE_SPEED 100.0f
#define MOVE_ACCEL 50.0f
#define MOVE_SPEED_AGRESSIVE 60.0f
#define MOVE_SPEED_LINK_DEATH 100.0f
#define MOVE_SPEED_THRESHOLD 5.0f
#define MOVE_SPEED_THRUSTER_TRESHOLD 20.0f
#define ROTATE_SPEED Calc::DegToRad(20.0f)
#define ROTATE_ACCEL Calc::DegToRad(10.0f)
#define ROTATE_SPEED_THRESHOLD Calc::DegToRad(10.0f)
#define BORDER_SIZE 300.0f
#define BORDER_SEGMENT_RADIUS 250.0f
#define BORDER_CORNER_RADIUS 140.0f
#define AVOID_DISTANCE 300.0f
#define MAXED_LEVEL 20

namespace Bandits
{
	EntityBandit::EntityBandit() : Game::Entity()
	{
		m_Log = LogCtx("EntityBandit");
	}
	
	EntityBandit::~EntityBandit()
	{
		delete m_Bandit;
		m_Bandit = 0;
	}
	
	void EntityBandit::Initialize()
	{
		Entity::Initialize();
		
		// entity overrides
		
		Class_set(Game::EntityClass_MaxiBoss);
		HitPoints_set(1000);
		Position_set(WORLD_MID);
		Flag_Set(Game::EntityFlag_IsMaxiBoss);
		Flag_Set(Game::EntityFlag_RenderAdditive);
		
		// bandit
		
		m_State = State_Dead;
		m_Bandit = 0;
		
		// bandit: movement
		
		m_PulseMoveSpeed.SetZero();
		m_PulseMoveFalloff = 0.0f;
		m_PulseRotateSpeed = 0.0f;
		m_PulseRotateFalloff = 0.0f;
		m_MoveDir.SetZero();
		m_MoveSpeedMax = 0.0f;
		m_MoveSpeed = 0.0f;
		m_MoveAccel = 0.0f;
		m_MoveFalloff = 0.0f;
		m_RotateDir = 0.0f;
		m_RotateSpeedMax = 0.0f;
		m_RotateSpeed = 0.0f;
		m_RotateAccel = 0.0f;
		m_RotateFalloff = 0.0f;
		m_Beam_AttackActive = false;
		
		//
		
		m_LinkRotationSpeedFactor = 0.0f;
		m_MissileHuntTime = 1.0f;
		
		// bandit: sequence
		
		m_Sequence.Initialize(this);
		
		//
		
		State_set(State_EnterScene);
	}
	
	void EntityBandit::Setup(Res* res, int level, float rotationBase, int mods)
	{
		m_Level = level;
		m_Mods = mods;
		
		m_LinkRotationSpeedFactor = 1.0f / (float)MAXED_LEVEL * (float)level;
	
		if (m_LinkRotationSpeedFactor > 3.0f)
			m_LinkRotationSpeedFactor = 3.0f;
		
		m_MissileHuntTime = Calc::Mid(1.0f + 2.0f * (float)level / (float)MAXED_LEVEL, 0.0f, 4.0f);
		
		m_BeamActivationBase =  Calc::Max(4.0f, 12.0f - 8.0f * (float)level / (float)MAXED_LEVEL);
		m_BeamActivationDeviation = Calc::Max(2.0f, 6.0f - 4.0f * (float)level / (float)MAXED_LEVEL);
		
		// load bandit
		
		BanditReader reader;
		
		VRCC::CompiledComposition* cc = (VRCC::CompiledComposition*)res->data;
		
		Assert(m_Bandit == 0);
		m_Bandit = reader.Read(this, cc, level);
		
		DoPosition_set(Position_get());
		
		if (rotationBase >= 0.0f)
			Rotation_set(rotationBase);
		else
			Rotation_set(Vec2F::ToAngle(m_EnterDirection));
		
		UpdateRotation(0.0f);
		
		// start beam attack timer
		
		m_Beam_ActivationTrigger.Start(GetRandomBeamActivationTime());
	}
	
	void EntityBandit::Update(float dt)
	{
		Entity::Update(dt);
		
		UpdateRotation(dt);
		UpdateMove(dt);
		
		m_Sequence.Update(dt);
		
		switch (m_State)
		{
			case State_EnterScene:
			{
				Speed_set(m_EnterDirection * ENTER_SPEED);
				
				DoPosition_set(Position_get() + Speed_get() * dt);
				
				m_Bandit->Update(dt);
				
				if (m_EnterEndTrigger.Read())
				{
					State_set(State_Alive);
				}
				
				break;
			}
			case State_Alive:
			{
				// update movement
				
				UpdateMovement(dt);
				
				// update coordinated attacks
				
				UpdateAttack(dt);
				
				// update bandit
				
				m_Bandit->Update(dt);
				
				//
				
				if (!m_Bandit->RootLink_get()->IsAlive_get())
					State_set(State_DieSequence);
				
				break;
			}
			case State_DieSequence:
			{
				if (!m_IsDestroyed)
				{
					// update movement
					
					UpdateMovement(dt);
					
					m_Bandit->RootLink_get()->UpdateTransform();
				}
				
				//
				
				if (!m_Sequence.IsRunning_get())
				{
					State_set(State_Dead);
				}
				
				break;
			}
			case State_Dead:
			{
				break;
			}
		}
	}
	
	void EntityBandit::UpdateSB(SelectionBuffer* sb)
	{
		m_Bandit->UpdateSB(sb);
	}
	
	void EntityBandit::Render_Below()
	{
		switch (m_State)
		{
			case State_EnterScene:
			case State_Alive:
				m_Bandit->Render_Below();
				break;
			case State_DieSequence:
			case State_Dead:
				break;
		}
	}
	
	void EntityBandit::Render()
	{
		if (g_GameState->DrawMode_get() != VectorShape::DrawMode_Texture)
			return;
		
		m_Sequence.Render_Back();
		
		switch (m_State)
		{
			case State_EnterScene:
			{
				m_Bandit->Render();
				break;
			}
			case State_Alive:
			{
				m_Bandit->Render();
				break;
			}
			case State_DieSequence:
			{
				if (!m_IsDestroyed)
				{
					m_Bandit->RootLink_get()->DoRender();
				}
				break;
			}
			case State_Dead:
			{
				break;
			}
		}
		
		m_Sequence.Render_Front();
		
		Render_DBG();
	}
	
	void EntityBandit::Render_Additive()
	{
		switch (m_State)
		{
			case State_EnterScene:
			{
				m_Bandit->m_LaserBeamMgr.Render();
				break;
			}
			
			case State_Alive:
			{
				m_Bandit->m_LaserBeamMgr.Render();
				break;
			}
			
			case State_DieSequence:
			{
				break;
			}
			
			case State_Dead:
			{
				break;
			}
		}
	}
	
	void EntityBandit::Render_DBG()
	{
#ifdef DEBUG
//		g_GameState->DBG_Console_WriteLine("bandit: state: %d", (int)m_State);
//		g_GameState->DBG_Console_WriteLine("bandit: rot_base: %.2f", m_RotationBase);
//		g_GameState->DBG_Console_WriteLine("bandit: rot_anim: %.2f", m_RotationAnim);
#endif
	}
	
	void EntityBandit::DoPosition_set(const Vec2F& pos)
	{
		Position_set(pos);
	
		if (m_Bandit)
		{
			m_Bandit->RootLink_get()->m_LocalPosition = pos;
		}
	}

	void EntityBandit::DoRotation_set(float angle)
	{
		Rotation_set(angle);
		
		if (m_Bandit)
		{
			m_Bandit->RootLink_get()->Rotation_set(angle);
		}
	}
	
	void EntityBandit::HandleDie()
	{
		Game::g_World->HandleKill(this);
	}
	
	bool EntityBandit::IsBeamActive_get() const
	{
		if (!m_Beam_AttackActive)
			return false;
		
		return m_Beam_DeactivationTrigger.IsRunning_get() != XFALSE;
	}
	
	bool EntityBandit::IsThrusterEnabled_get() const
	{
		if (!m_MoveTrigger.IsRunning_get())
			return false;
		
		return m_MoveSpeed >= MOVE_SPEED_THRUSTER_TRESHOLD;
	}
	
	float EntityBandit::ThrusterAngle_get() const
	{
		return Vec2F::ToAngle(m_MoveDir);
	}
	
	float EntityBandit::LinkRotationSpeedFactor_get() const
	{
		return m_LinkRotationSpeedFactor;
	}
	
	float EntityBandit::MissileHuntTime_get() const
	{
		return m_MissileHuntTime;
	}
	
	float EntityBandit::GetRandomBeamActivationTime() const
	{
		return m_BeamActivationBase + Calc::Random(m_BeamActivationDeviation);
	}
	
	void EntityBandit::State_set(State state)
	{
		m_State = state;
		
		switch (m_State)
		{
			case State_EnterScene:
			{
				DoPosition_set(CreateSpawnPoint());
				
				Vec2F pos = Position_get();
				
				if (pos[0] < 0.0f)
					m_EnterDirection[0] = 1.0f;
				if (pos[1] < 0.0f)
					m_EnterDirection[1] = 1.0f;
				if (pos[0] > WORLD_SX)
					m_EnterDirection[0] = -1.0f;
				if (pos[1] > WORLD_SY)
					m_EnterDirection[1] = -1.0f;
				
				m_EnterDirection.Normalize();
				
				m_EnterEndTrigger.Start(4.0f);
				
				break;
			}
			case State_Alive:
			{
				// start move & rotate triggers
				m_MoveTrigger.Start(Calc::Random(0.0f, 5.0f));
				m_RotateTrigger.Start(Calc::Random(0.0f, 5.0f));
				break;
			}
			case State_DieSequence:
			{
				Game::g_World->HandleBanditDeath(this);
				Flag_Set(Game::EntityFlag_Transient);
				m_IsDestroyed = false;
				m_Sequence.StartDestruction();
				break;
			}
			case State_Dead:
			{
				Flag_Set(Game::EntityFlag_DidDie);
				
				break;
			}
		}
	}
	
	// ====================
	// State: Alive
	// ====================
	
	// --------------------
	// Movement
	// --------------------
	
	void EntityBandit::UpdateMovement(float dt)
	{
		if (m_State == State_Alive)
		{
			m_MoveTrigger.Read();
			m_RotateTrigger.Read();
			
			if (MoveHasEnded_get() && !m_MoveTrigger.IsRunning_get())
			{
				bool done = false;
				
				if (m_Mods & BanditMod_AvoidBandits)
				{
					Vec2F dir;
					
					if (EvaluateBanditAvoid(dir))
					{
						m_Log.WriteLine(LogLevel_Debug, "avoid. dir: %f, %f", dir[0], dir[1]);
						Make_Move(dir * MOVE_SPEED, MOVE_ACCEL, 0.4f, 3.0f);
						m_MoveTrigger.Start(GetMoveInterval());
						done = true;
					}
				}
				
				if (!done)
				{
					Vec2F dim = WORLD_S - Vec2F(BORDER_SIZE * 2.0f, BORDER_SIZE * 2.0f);
					Vec2F target(BORDER_SIZE + Calc::Random(dim[0]), BORDER_SIZE + Calc::Random(dim[1]));
					m_Log.WriteLine(LogLevel_Debug, "target: %f, %f", target[0], target[1]);
					Vec2F dir = (target - Position_get()).Normal();
					Make_Move(dir * MOVE_SPEED, MOVE_ACCEL, 0.4f, 5.0f);
					m_MoveTrigger.Start(GetMoveInterval());
				}
			}
			
			if (RotateHasEnded_get() && !m_RotateTrigger.IsRunning_get())
			{
				float maxSpeed = ROTATE_SPEED * (Calc::Random() % 2 == 0 ? -1 : +1);
				float duration = Calc::Random(2.0f, 5.0f);
				Make_Rotate(maxSpeed, ROTATE_ACCEL, 0.4f, duration);
				m_RotateTrigger.Start(GetRotateInterval());
			}
		}
		
		Vec2F pos = Position_get();
		
		float pushAccel = 70.0f;
		
		if (pos[0] < BORDER_SIZE)
			Speed_set(Speed_get() + Vec2F(+pushAccel, 0.0f) * dt);
		if (pos[1] < BORDER_SIZE)
			Speed_set(Speed_get() + Vec2F(0.0f, +pushAccel) * dt);
		if (pos[0] > WORLD_SX - BORDER_SIZE)
			Speed_set(Speed_get() + Vec2F(-pushAccel, 0.0f) * dt);
		if (pos[1] > WORLD_SY - BORDER_SIZE)
			Speed_set(Speed_get() + Vec2F(0.0f, -pushAccel) * dt);
		
		DoPosition_set(Position_get() + Speed_get() * dt);
		
		Speed_set(Speed_get() * powf(0.25f, dt));
	}
	
	void EntityBandit::UpdateMove(float dt)
	{
		m_MoveEndTrigger.Read();
		
		m_MoveSpeed *= powf(m_MoveFalloff, dt);
		if (m_MoveEndTrigger.IsRunning_get())
			m_MoveSpeed += m_MoveAccel * dt;
		if (m_MoveSpeed > m_MoveSpeedMax)
			m_MoveSpeed = m_MoveSpeedMax;
		
		m_PulseMoveSpeed *= powf(m_PulseMoveFalloff, dt);
		
//		m_Log.WriteLine(LogLevel_Debug, "move speed: %f", m_MoveSpeed);
		
		DoPosition_set(Position_get() + m_MoveDir * m_MoveSpeed * dt + m_PulseMoveSpeed * dt);
	}
	
	void EntityBandit::UpdateRotation(float dt)
	{
		m_RotateEndTrigger.Read();
		
		m_RotateSpeed *= powf(m_RotateFalloff, dt);
		if (m_RotateEndTrigger.IsRunning_get())
			m_RotateSpeed += m_RotateAccel * dt;
		if (m_RotateSpeed > m_RotateSpeedMax)
			m_RotateSpeed = m_RotateSpeedMax;

		m_PulseRotateSpeed *= powf(m_PulseRotateFalloff, dt);
		
//		m_Log.WriteLine(LogLevel_Debug, "rotate speed: %f", m_RotateSpeed);
		
		DoRotation_set(Rotation_get() + m_RotateDir * m_RotateSpeed * dt + m_PulseRotateSpeed * dt);
	}
		
	bool EntityBandit::MoveHasEnded_get() const
	{
		return !m_MoveEndTrigger.IsRunning_get() && m_MoveSpeed <= MOVE_SPEED_THRESHOLD;
	}
	
	bool EntityBandit::RotateHasEnded_get() const
	{
		return !m_RotateEndTrigger.IsRunning_get() && m_RotateSpeed <= ROTATE_SPEED_THRESHOLD;
	}
	
	void EntityBandit::Make_Move(Vec2F speed, float accel, float falloff, float duration)
	{
		m_MoveDir = speed.Normal();
		m_MoveSpeedMax = speed.Length_get();
		m_MoveSpeed = 0.0f;
		m_MoveAccel = accel;
		m_MoveFalloff = falloff;
		m_MoveEndTrigger.Start(duration);
	}
	
	void EntityBandit::Make_MoveAgressive(Vec2F target, float speed)
	{
		Vec2F delta = target - Position_get();
		
		PulseMove(delta.Normal() * speed, 0.4f);
	}
	
	void EntityBandit::Make_MoveRetreat(Vec2F opponent, float speed)
	{
		Vec2F delta = opponent - Position_get();
		
		Vec2F dir = Vec2F::FromAngle(Vec2F::ToAngle(delta) + Calc::mPI2);
		
		PulseMove(dir * speed, 0.4f);
	}
	
	void EntityBandit::Make_Rotate(float maxSpeed, float accel, float falloff, float duration)
	{
		m_RotateDir = Calc::Sign(maxSpeed);
		m_RotateSpeedMax = Calc::Abs(maxSpeed);
		m_RotateSpeed = 0.0f;
		m_RotateAccel = accel;
		m_RotateFalloff = falloff;
		m_RotateEndTrigger.Start(duration);
	}
	
	//
	
	void EntityBandit::PulseMove(Vec2F speed, float falloff)
	{
		m_PulseMoveSpeed += speed;
		m_PulseMoveFalloff = falloff;
	}
	 
	void EntityBandit::PulseRotate(float speed, float falloff)
	{
		m_PulseRotateSpeed += speed;
		m_PulseRotateFalloff = falloff;
	}
	
	float EntityBandit::GetMoveInterval() const
	{
		if (m_Bandit->RootLink_get()->m_InsulationFactor > 1)
			return Calc::Random(10.0f, 20.0f);
		else
			return 5.0f;
	}
	
	float EntityBandit::GetRotateInterval() const
	{
		return Calc::Random(10.0f, 20.0f);
	}
	
	// --------------------
	// Attack
	// --------------------
	void EntityBandit::UpdateAttack(float dt)
	{
		if (m_Beam_DeactivationTrigger.Read())
		{
			Attack_Beam_Stop(GetRandomBeamActivationTime());
		}
		
		if (m_Beam_ActivationTrigger.Read())
		{
			SoundEffectInfo* sound1 = (SoundEffectInfo*)(g_GameState->GetSound(Resources::SOUND_MAXI_BEAM_CHARGE)->info);
			SoundEffectInfo* sound2 = (SoundEffectInfo*)(g_GameState->GetSound(Resources::SOUND_MAXI_BEAM_FIRE)->info);
			
			float duration = sound1->Duration_get() + sound2->Duration_get();
			
			Attack_Beam_Start(duration);
		}
	}
	
	void EntityBandit::Attack_Beam_Start(float duration)
	{
		m_Beam_AttackActive = m_Bandit->StartBeamAttack();
		
		m_Beam_ActivationTrigger.Stop();
		
		m_Beam_DeactivationTrigger.Start(duration);
	}
	
	void EntityBandit::Attack_Beam_Stop(float interval)
	{
		m_Bandit->StopBeamAttack();
		
		m_Beam_DeactivationTrigger.Stop();
		
		m_Beam_ActivationTrigger.Start(interval);
	}
	
	// --------------------
	// Support
	// --------------------
	
	Vec2F EntityBandit::CreateSpawnPoint() const
	{
		Vec2F pos;
		
		bool stop = false;
		
		while (!stop)
		{
			bool consider = true;
			
			pos = Game::g_World->MakeSpawnPoint_OutsideWorld(0.0f);
			
			Vec2F corner[4] =
			{
				Vec2F(0.0f, 0.0f),
				Vec2F(WORLD_SX, 0.0f),
				Vec2F(WORLD_SX, WORLD_SY),
				Vec2F(0.0f, WORLD_SY)
			};
			
			for (int i = 0; i < 4; ++i)
			{
				float distance2corner = (pos - corner[i]).Length_get();
				
				if (distance2corner < BORDER_CORNER_RADIUS)
					consider = false;
			}
			
			float distance = (Game::g_World->m_Player->Position_get() - pos).Length_get();
			
			if (distance < WORLD_SX * 0.5f)
				consider = false;
			
			if (consider)
			{
				stop = true;
			}
		}
		
		float radius = CalculateRadius() + BORDER_SEGMENT_RADIUS;
		
		Vec2F dir = GetSpawnDir(pos);
		
		pos -= dir * radius;
		
		return pos;
	}
	
	Vec2F EntityBandit::GetSpawnDir(const Vec2F& pos) const
	{
		float eps = 10.0f;
		
		if (pos[0] >= -eps && pos[0] <= +eps)
			return Vec2F(+1.0f, 0.0f);
		if (pos[1] >= -eps && pos[1] <= +eps)
			return Vec2F(0.0f, +1.0f);
		if (pos[0] >= WORLD_SX - eps && pos[0] <= WORLD_SX + eps)
			return Vec2F(-1.0f, 0.0f);
		if (pos[1] >= WORLD_SY - eps && pos[1] <= WORLD_SY + eps)
			return Vec2F(0.0f, -1.0f);
		
		return Vec2F(0.0f, 0.0f);
	}
	
	float EntityBandit::CalculateRadius() const
	{
//		float max = 0.0f;
		// todo..
		
		return 200.0f;
	}
	
	bool EntityBandit::IsInsideWorld(const Vec2F& pos) const
	{
		float border = BORDER_SIZE;
		
		RectF worldRect(Vec2F(border, border), WORLD_S - Vec2F(border, border) * 2.0f);
		
		return worldRect.IsInside(pos) != XFALSE;
	}
	
	class AvoidQuery
	{
	public:
		AvoidQuery()
		{
			self = 0;
			avoid = false;
		}
		
		const EntityBandit* self;
		Vec2F dir;
		bool avoid;
	};
	
	static void HandleAvoidQuery(void* obj, void* arg)
	{
		AvoidQuery* query = (AvoidQuery*)obj;
		
		Game::Entity* entity = (Game::Entity*)arg;
		
		if (entity->Class_get() == Game::EntityClass_MaxiBoss)
		{
			if (entity == query->self)
				return;
			
			Vec2F delta = query->self->Position_get() - entity->Position_get();
			
			float distanceSq = delta.LengthSq_get();
			
			if (distanceSq > 0.0f && distanceSq < AVOID_DISTANCE * AVOID_DISTANCE)
			{
				Vec2F weight = delta / distanceSq;
				
				query->dir += weight;
				query->avoid = true;
			}
		}
	}
	
	bool EntityBandit::EvaluateBanditAvoid(Vec2F& oDir) const
	{
		AvoidQuery query;
		
		query.self = this;
		
		Game::g_World->ForEachDynamic(CallBack(&query, HandleAvoidQuery));
		
		if (query.avoid)
			oDir = query.dir.Normal();
		
		return query.avoid;
	}
	
	void EntityBandit::ForEach_Link(CallBack cb)
	{
		Link** linkArray = m_Bandit->LinkArray_get();
		
		for (int i = 0; i < m_Bandit->LinkArraySize_get(); ++i)
		{
			if (!linkArray[i]->IsAlive_get())
				continue;
			
			cb.Invoke(linkArray[i]);
		}
	}
	
	// --------------------
	// Death sequence
	// --------------------
	void EntityBandit::Destroy()
	{
		m_IsDestroyed = true;
	}
	
	// --------------------
	// Bandit
	// --------------------
	void EntityBandit::HandleDeath(Bandits::Link* link)
	{
		if (link->m_LinkType == LinkType_Segment)
			Game::g_World->HandleKill(link);
		
		switch (link->m_LinkType)
		{
			case LinkType_Core:
			{
				Game::Reward reward(Game::RewardType_ScorePlatinum);
				Game::g_World->m_ScoreEffectMgr.Spawn(reward, link->GlobalPosition_get() - Vec2F(0.0f, 10.0f));
				Game::g_World->m_Player->HandleReward(reward);
				break;
			}
			case LinkType_Segment:
			{
				Vec2F dir = (Position_get() - link->m_GlobalPosition).Normal();
				PulseMove(dir * MOVE_SPEED_LINK_DEATH, 0.2f);
				
				Game::Reward reward(Game::RewardType_ScoreGold);
				Game::g_World->m_ScoreEffectMgr.Spawn(reward, link->GlobalPosition_get() - Vec2F(0.0f, 10.0f));
				Game::g_World->m_Player->HandleReward(reward);
				
				if (m_Bandit->RootLink_get()->m_InsulationFactor == 1)
				{
					m_Bandit->RootLink_get()->BeginPanicAttack();
				}
				break;
			}
			case LinkType_Weapon:
			{
				Game::Reward reward(Game::RewardType_ScoreSilver);
				Game::g_World->m_ScoreEffectMgr.Spawn(reward, link->GlobalPosition_get() - Vec2F(0.0f, 10.0f));
				Game::g_World->m_Player->HandleReward(reward);
				break;
			}
			case LinkType_Undefined:
				break;
		}
	}
}
