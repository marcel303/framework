#include "Debug.h"
#include "Engine.h"
#include "Entity.h"
#include "PhyScene.h"
#include "Scene.h"

EntityFactory g_entityFactory;

Entity::Entity()
{
	SetClassName("Entity");

	m_entity_NS = new Entity_NS(this);

	m_client = 0;
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
	Assert(m_scene == 0);

	delete m_entity_NS;
	m_entity_NS = 0;
}

ShEntity Entity::Self()
{
	Assert(m_scene);

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
}

void Entity::UpdateAnimation(float dt)
{
	if (m_caps & CAP_PHYSICS)
	{
		Mat4x4 transform;

		transform.MakeTranslation(m_phyObject.m_position);

		SetTransform(transform);

		m_entity_NS->SetDirty(); // todo : check net flags
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

		m_scene->GetPhyScene()->AddObject(object);

		if (m_caps & CAP_DYNAMIC_PHYSICS)
		{
			for (size_t j = 0; j < object->GetGeometry().size(); ++j)
				m_cdFlt.DenyObject(object->GetGeometry()[j].get());
		}
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
