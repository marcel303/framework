#include "EntityMissile.h"

EntityMissile::EntityMissile(Vec3 position, Vec3 speed, Entity* target) : Entity()
{
	SetClassName("Missile");
	//EnableCaps(CAP_PHYSICS);

	AddParameter(Parameter(PARAM_INT32, "target", REP_ONCE, COMPRESS_NONE, &m_targetID));
	AddParameter(Parameter(PARAM_INT32, "px", REP_CONTINUOUS, COMPRESS_NONE, &m_phyObject.m_position[0]));
	AddParameter(Parameter(PARAM_INT32, "py", REP_CONTINUOUS, COMPRESS_NONE, &m_phyObject.m_position[1]));
	AddParameter(Parameter(PARAM_INT32, "pz", REP_CONTINUOUS, COMPRESS_NONE, &m_phyObject.m_position[2]));
	AddParameter(Parameter(PARAM_INT32, "vx", REP_CONTINUOUS, COMPRESS_NONE, &m_phyObject.m_velocity[0]));
	AddParameter(Parameter(PARAM_INT32, "vy", REP_CONTINUOUS, COMPRESS_NONE, &m_phyObject.m_velocity[1]));
	AddParameter(Parameter(PARAM_INT32, "vz", REP_CONTINUOUS, COMPRESS_NONE, &m_phyObject.m_velocity[2]));

	m_position = position;
	m_speed = speed;
	m_target = target;

	m_life = 5.0f;
}

EntityMissile::~EntityMissile()
{
}

void EntityMissile::PostCreate()
{
	m_phyObject.Initialize(1, m_position, m_speed, true, false, this);

	if (m_target)
		m_targetID = m_target->m_id;
	else
		m_targetID = 0;

	//AddPhyObject(&m_phyObject);

	ShapeBuilder sb;
	sb.PushScaling(Vec3(1.0f, 1.0f, 1.0f));
	sb.CreateCube(&g_alloc, m_mesh);
	sb.Pop();
}

void EntityMissile::UpdateLogic(float dt)
{
	Entity::UpdateLogic(dt);

	m_life -= dt;

	ShEntity targetEnt = m_scene->FindEntity(m_targetID);

	if (targetEnt.get() == 0)
	{
		m_life = 0.0f;
	}
	else
	{
		if (targetEnt->m_className == "Player")
		{
			Player* player = (Player*)targetEnt.get();

			Vec3 delta = player->m_phyObject.m_position - m_phyObject.m_position;

			//if (delta.CalcSize() < 50.0f)
				//m_life = 0.0f;
		}
	}

	if (m_life <= 0.0f)
		m_scene->RemoveEntityQueued(Self());
}

void EntityMissile::UpdateAnimation(float dt)
{
	Entity::UpdateAnimation(dt);

	ShEntity targetEnt = m_scene->FindEntity(m_targetID);

	if (targetEnt.get())
	{
		if (targetEnt->m_className == "Player")
		{
			Player* player = (Player*)targetEnt.get();

			Vec3 delta = player->m_phyObject.m_position - m_phyObject.m_position;

			// FIXME, haxxorz.
			m_phyObject.m_force = delta;
			m_phyObject.m_velocity += m_phyObject.m_force * dt;
			m_phyObject.m_velocity *= pow(0.5f, dt);
			m_phyObject.m_position += m_phyObject.m_velocity * dt;
		}
	}
}

void EntityMissile::Render()
{
	Entity::Render();

	//circle(Render::g_bitmap, m_phyObject.m_position[0], m_phyObject.m_position[2], 3, makecol(255, 0, 0));

	Renderer::I().GetGraphicsDevice()->SetTex(0, 0);

	Mat4x4 matW;
	matW.MakeTranslation(m_phyObject.m_position);
	Renderer::I().MatW().Push(matW);
	Renderer::I().RenderMesh(m_mesh);
	Renderer::I().MatW().Pop();
}
