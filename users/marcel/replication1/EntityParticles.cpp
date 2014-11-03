#include "EntityParticles.h"

EntityParticles::EntityParticles(Vec3 position, int particleCount, float size) : Entity()
{
	SetClassName("Particles");

	AddParameter(Parameter(PARAM_INT32, "pc", REP_ONCE, COMPRESS_NONE, &m_particleCount));
	AddParameter(Parameter(PARAM_FLOAT32, "px", REP_ONCE, COMPRESS_NONE, &m_position[0]));
	AddParameter(Parameter(PARAM_FLOAT32, "py", REP_ONCE, COMPRESS_NONE, &m_position[1]));
	AddParameter(Parameter(PARAM_FLOAT32, "pz", REP_ONCE, COMPRESS_NONE, &m_position[2]));
	AddParameter(Parameter(PARAM_FLOAT32, "ps", REP_ONCE, COMPRESS_NONE, &m_size));

	m_position = position;
	m_particleCount = particleCount;
	m_size = size;
}

EntityParticles::~EntityParticles()
{
}

void EntityParticles::Render()
{
	m_psys.Render();
}

void EntityParticles::UpdateLogic(float dt)
{
	if (m_psys.AllDead())
		m_scene->RemoveEntityQueued(m_id);
}

void EntityParticles::UpdateAnimation(float dt)
{
	m_psys.Update(dt);
}

void EntityParticles::UpdateRender()
{
	m_shader = m_psys.m_shader;
}

void EntityParticles::OnSceneAdd(Scene* scene)
{
	Entity::OnSceneAdd(scene);

	m_psys.Initialize(m_position, m_particleCount, false);
	m_psys.m_src->Init(m_size, m_size, 1.0f, 0.1f);
}
