#include "Calc.h"
#include "ParticleSys.h"
#include "Renderer.h"
#include "ResMgr.h"
#include "Timer.h"

Particle::Particle()
{
	m_size = 1.0f;
	m_life = 0.0f;
	m_lifeSpan = 1.0f;
}

void Particle::Update(float dt)
{
	m_velocity += m_force * dt;
	m_position += m_velocity * dt;
	m_life -= dt / m_lifeSpan;
}

bool Particle::IsDead() const
{
	return m_life <= 0.0f;
}

ParticleSrc::ParticleSrc()
{
	m_emitTimer.Initialize(&g_TimerRT);

	Init(1.0f, 1.0f, 1.0f, 1.0f);
}

ParticleSrc::~ParticleSrc()
{
}

bool ParticleSrc::WantEmit()
{
	return m_emitTimer.PeekTick();
}

void ParticleSrc::Emit(Particle* particle, const Vec3& position, float t)
{
	m_emitTimer.ReadTick();

	particle->m_lifeSpan = m_lifeSpan;

	particle->m_position = position;

	particle->m_velocity[0] = Calc::Random(-0.5f, +0.5f) * m_speed;
	particle->m_velocity[1] = Calc::Random(-0.5f, +0.5f) * m_speed;
	particle->m_velocity[2] = Calc::Random(-0.5f, +0.5f) * m_speed;

	particle->m_size = m_size;
}

void ParticleSrc::Init(float speed, float size, float lifeSpan, float interval)
{
	FASSERT(interval > 0.0f);

	m_speed = speed;
	m_size = size;
	m_lifeSpan = lifeSpan;
	m_interval = interval;

	m_emitTimer.SetFrequency(1.0f / interval);
	m_emitTimer.Restart();
}

ParticleSrcCone::ParticleSrcCone() : ParticleSrc()
{
	Init(1.0f, 1.0f, 1.0f, 1.0f, Vec3(0.0f, 1.0f, 0.0f), Calc::mPI / 2.0f);
}

void ParticleSrcCone::Emit(Particle* particle, const Vec3& position, float t)
{
	ParticleSrc::Emit(particle, position, t);

	float angle1 = Calc::Random(0.0f, m_aperture / 2.0f);
	float angle2 = Calc::Random(0.0f, Calc::mPI * 2.0f);

	Mat4x4 rot1;
	Mat4x4 rot2;
	Mat4x4 rot3;

	rot1.MakeRotationX(angle1);
	rot2.MakeRotationZ(angle2);

	Vec3 up;

	while (up.CalcSizeSq() == 0.0f)
	{
		Vec3 r(Calc::Random(-1.0f, +1.0f), Calc::Random(-1.0f, +1.0f), Calc::Random(-1.0f, +1.0f));
		up = m_direction % r;
	}

	up.Normalize();

	rot3.MakeLookat(Vec3(0.0f, 0.0f, 0.0f), m_direction, up);

	Mat4x4 mat = rot3.CalcInv() * rot2 * rot1;

	particle->m_velocity = mat * Vec3(0.0f, 0.0f, m_speed);
}

void ParticleSrcCone::Init(float speed, float size, float lifeSpan, float interval, Vec3 direction, float aperture)
{
	ParticleSrc::Init(speed, size, lifeSpan, interval);

	m_direction = direction;
	m_aperture = aperture;
}

ParticleMod::ParticleMod()
{
	SetGravity(Vec3(0.0f, -10.0f, 0.0f));
}

void ParticleMod::Mod(Particle* particle, float t)
{
	particle->m_force = m_gravity;
}

void ParticleMod::SetGravity(Vec3 gravity)
{
	m_gravity = gravity;
}

ParticleSys::ParticleSys()
{
	m_respawn = false;

	m_src = new ParticleSrc();
	m_mod = new ParticleMod();

	//m_tex = RESMGR.GetTex("textures/particle1.png");
	m_tex = RESMGR.GetTex("textures/particle2.png");
	m_shader = ShShader(new ResShader());
	m_shader->InitBlend(true, BLEND_ONE, BLEND_ONE);
	m_shader->InitWriteDepth(false);
	m_shader->m_vs = RESMGR.GetVS("shaders/particle_vs.cg");
	m_shader->m_ps = RESMGR.GetPS("shaders/particle_ps.cg");

	m_t = 0.0f;
}

ParticleSys::~ParticleSys()
{
	delete m_src;
	delete m_mod;
}

void ParticleSys::Update(float dt)
{
	while (m_src->WantEmit())
	{
		int index = -1;

		for (int i = 0; i < m_particles.GetSize() && index == -1; ++i)
			if ((m_respawn && m_particles[i].IsDead()) || m_particles[i].m_life == 2.0f)
				index = i;

		if (index != -1)
		{
			m_particles[index] = Particle();
			m_particles[index].m_life = 1.0f;
			m_src->Emit(&m_particles[index], m_position, m_t);
		}
		else
		{
			Particle temp;
			m_src->Emit(&temp, m_position, m_t);
		}
	}

	for (int i = 0; i < m_particles.GetSize(); ++i)
	{
		m_mod->Mod(&m_particles[i], m_t);

		m_particles[i].Update(dt);
	}

	m_t += dt;
}

void ParticleSys::Render()
{
	FillVB();

	GraphicsDevice* gfx = Renderer::I().GetGraphicsDevice();

	gfx->SetVB(&m_vb);
	gfx->SetIB(&m_ib);

	// FIXME, not compatible with render list.
	m_shader->m_vs->p["wv"] =
		Renderer::I().MatV().Top() *
		Renderer::I().MatW().Top();
	m_shader->m_vs->p["p"] =
		Renderer::I().MatP().Top();
	m_shader->m_ps->p["tex"] = m_tex.get();

	m_shader->Apply(gfx);

	gfx->Draw(PT_TRIANGLE_LIST);

	//m_shader->InitBlend(false, BLEND_SRC, BLEND_DST);
	m_shader->InitWriteDepth(true);
	m_shader->Apply(gfx);
	//m_shader->InitBlend(true, BLEND_ONE, BLEND_ONE);
	m_shader->InitWriteDepth(false);
}

void ParticleSys::Initialize(Vec3 position, int particleCount, bool respawn)
{
	m_position = position;
	m_respawn = respawn;

	m_particles.SetSize(particleCount);

	for (int i = 0; i < particleCount; ++i)
	{
		m_particles[i].m_life = 2.0f;
	}

	int indexCount = particleCount * 6;
	int vertexCount = particleCount * 4;
	
	m_ib.Initialize(&g_alloc, indexCount);
	m_vb.Initialize(&g_alloc, vertexCount, FVF_XYZ | FVF_TEX2);

	for (int i = 0; i < particleCount; ++i)
	{
		int index0 = i * 4;

		int index1 = index0 + 0;
		int index2 = index0 + 1;
		int index3 = index0 + 2;

		int index4 = index0 + 0;
		int index5 = index0 + 2;
		int index6 = index0 + 3;

		m_ib.index[i * 6 + 0] = index1;
		m_ib.index[i * 6 + 1] = index2;
		m_ib.index[i * 6 + 2] = index3;

		m_ib.index[i * 6 + 3] = index4;
		m_ib.index[i * 6 + 4] = index5;
		m_ib.index[i * 6 + 5] = index6;
	}
}

bool ParticleSys::AllDead()
{
	for (int i = 0; i < m_particles.GetSize(); ++i)
		if (!m_particles[i].IsDead())
			return false;

	return true;
}

void ParticleSys::FillVB()
{
	for (int i = 0; i < m_particles.GetSize(); ++i)
	{
		Vec3 p = m_position + m_particles[i].m_position;
		float s = m_particles[i].IsDead() ? 0.0f : (m_particles[i].m_size / 2.0f * m_particles[i].m_life);

		m_vb.position[i * 4 + 0] = p;
		m_vb.position[i * 4 + 1] = p;
		m_vb.position[i * 4 + 2] = p;
		m_vb.position[i * 4 + 3] = p;

		m_vb.tex[0][i * 4 + 0] = Vec2(0.0f, 0.0f);
		m_vb.tex[0][i * 4 + 1] = Vec2(1.0f, 0.0f);
		m_vb.tex[0][i * 4 + 2] = Vec2(1.0f, 1.0f);
		m_vb.tex[0][i * 4 + 3] = Vec2(0.0f, 1.0f);

		m_vb.tex[1][i * 4 + 0] = Vec2(-s, -s);
		m_vb.tex[1][i * 4 + 1] = Vec2(-s, +s);
		m_vb.tex[1][i * 4 + 2] = Vec2(+s, +s);
		m_vb.tex[1][i * 4 + 3] = Vec2(+s, -s);
	}

	m_vb.Invalidate();
}
