#include "Debug.h"
#include "Engine.h"
#include "Entity.h"
#include "Scene.h"

Entity::Entity()
{
	SetClassName("Entity");
	AddParameter(Parameter(PARAM_INT32,   "id",    REP_ONCE,       COMPRESS_NONE, &m_id));
	// TODO: Need Vec3 replicated type.. too much overhead, due to onchange (adds indices).
	AddParameter(Parameter(PARAM_FLOAT32, "px", REP_CONTINUOUS, COMPRESS_NONE, &m_phyObject.m_position[0]));
	AddParameter(Parameter(PARAM_FLOAT32, "py", REP_CONTINUOUS, COMPRESS_NONE, &m_phyObject.m_position[1]));
	AddParameter(Parameter(PARAM_FLOAT32, "pz", REP_CONTINUOUS, COMPRESS_NONE, &m_phyObject.m_position[2]));
	AddParameter(Parameter(PARAM_SPEEDF, "vx", REP_CONTINUOUS, COMPRESS_8_8, &m_phyObject.m_velocity[0]));
	AddParameter(Parameter(PARAM_SPEEDF, "vy", REP_CONTINUOUS, COMPRESS_8_8, &m_phyObject.m_velocity[1]));
	AddParameter(Parameter(PARAM_SPEEDF, "vz", REP_CONTINUOUS, COMPRESS_8_8, &m_phyObject.m_velocity[2]));

	m_scene = 0;
	m_id = 0;
	m_active = false;
	m_caps = 0;
	m_repObjectID = 0;

	m_cdGroup = 0;

	m_cdFlt.Default(true);

	m_transform.MakeIdentity();

	m_shader = ShShader(new ResShader());

	m_hasAABB = false;
}

Entity::~Entity()
{
	FASSERT(m_scene == 0);
}

void Entity::PostCreate()
{
}

ShEntity Entity::Self()
{
	FASSERT(m_scene);

	return m_scene->FindEntity(m_id);
}

void Entity::EnableCaps(int caps)
{
	UpdateCaps(m_caps, m_caps | caps);
}

void Entity::DisableCaps(int caps)
{
	UpdateCaps(m_caps, m_caps & (~caps));
}

Scene* Entity::GetScene() const
{
	return m_scene;
}

int Entity::GetID() const
{
	return m_id;
}

Mat4x4 Entity::GetTransform() const
{
	if (m_caps & CAP_PHYSICS)
	{
		Mat4x4 matPos;

		matPos.MakeTranslation(m_phyObject.m_position);
		
		return matPos;
	}
	else
		return m_transform;
}

Vec3 Entity::GetPosition() const
{
	return GetTransform().Mul(Vec4(0.0f, 0.0f, 0.0f, 1.0f)).XYZ();
}

Vec3 Entity::GetOrientation() const
{
	return GetTransform().Mul(Vec4(0.0f, 0.0f, 1.0f, 0.0f)).XYZ();
}

ParameterList* Entity::GetParameters()
{
	return &m_parameters;
}
const std::string& Entity::GetClassName() const
{
	return m_className;
}

void Entity::SetClassName(const std::string& className)
{
	m_className = className;
}

void Entity::SetScene(Scene* scene)
{
	if (m_scene)
		OnSceneRemove(m_scene);

	m_scene = scene;

	if (m_scene)
		OnSceneAdd(m_scene);
}

void Entity::SetActive(bool active)
{
	if (m_active == active)
		return;

	m_active = active;

	if (m_active)
		OnActivate();
	else
		OnDeActivate();
}

void Entity::SetController(int id)
{
}

void Entity::SetTransform(const Mat4x4& transform)
{
	if (m_caps & CAP_PHYSICS)
	{
		Vec3 position = transform.GetTranslation();

		m_phyObject.m_position = position;
	}
	else
	{
		m_transform = transform;
	}

	OnTransformChange();
}

void Entity::UpdateLogic(float dt)
{
	//if (m_caps & CAP_DYNAMIC_PHYSICS)
	if (m_caps & CAP_PHYSICS)
	{
		// FIXME: Check for actual change to physics object's position/velo.
		/*
		m_parameters["px"]->Invalidate();
		m_parameters["py"]->Invalidate();
		m_parameters["pz"]->Invalidate();
		m_parameters["vx"]->Invalidate();
		m_parameters["vy"]->Invalidate();
		m_parameters["vz"]->Invalidate();*/
	}
}

void Entity::UpdateAnimation(float dt)
{
	if (m_caps & CAP_PHYSICS)
	{
		Mat4x4 transform;

		transform.MakeTranslation(m_phyObject.m_position);

		SetTransform(transform);
	}
}

void Entity::UpdateRender()
{
}

void Entity::Render()
{
}

void Entity::ReplicatedMessage(int message, int value)
{
	for (size_t i = 0; i < m_scene->m_engine->m_serverClients.size(); ++i)
		m_scene->MessageEntity(m_scene->m_engine->m_serverClients[i], m_id, message, value);
}

void Entity::OnSceneAdd(Scene* scene)
{
	if (m_caps & CAP_PHYSICS)
	{
		Phy::Object* object = &m_phyObject;

#if 0
		// FIXME: Should not be necessary.. :/
		for (size_t i = 0; i < object->GetGeometry().size(); ++i)
			object->GetGeometry()[i]->SetUP(this);
#endif

		m_scene->GetPhyScene()->AddObject(object);

		for (size_t j = 0; j < object->GetGeometry().size(); ++j)
			m_cdFlt.DenyObject(object->GetGeometry()[j].get());
	}

	if (m_aabbObjects.size() > 0)
	{
		m_aabb = m_aabbObjects[0]->CalcAABB();

		for (size_t i = 1; i < m_aabbObjects.size(); ++i)
			m_aabb += m_aabbObjects[i]->CalcAABB();

		m_hasAABB = true;
	}
	else
		m_hasAABB = false;
}

void Entity::OnSceneRemove(Scene* scene)
{
	if (m_caps & CAP_PHYSICS)
	{
		Phy::Object* object = &m_phyObject;

		m_scene->GetPhyScene()->RemoveObject(object);
	}
}

void Entity::OnActivate()
{
}

void Entity::OnDeActivate()
{
}

void Entity::OnMessage(int message, int value)
{
}

void Entity::OnTransformChange()
{
}

void Entity::AddParameter(const Parameter& parameter)
{
	m_parameters.Add(parameter);
}

void Entity::AddParameter(ShParameter parameter)
{
	m_parameters.Add(parameter);
}

void Entity::UpdateCaps(int oldCaps, int newCaps)
{
	int diff = oldCaps ^ newCaps;

	// TODO.
	if (diff)
	{
	}

	m_caps = newCaps;
}

void Entity::AddAABBObject(AABBObject* object)
{
	m_aabbObjects.push_back(object);
}
