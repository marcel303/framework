#include "Calc.h"
#include "Debugging.h"
#include "Engine.h"
#include "EntityPlayer.h"
#include "Scene.h"
#include "SceneRenderer.h"

#include "ControllerExample.h"

DEFINE_ENTITY(EntityPlayer, EntityPlayer);

EntityPlayer::EntityPlayer()
	: Entity()
{
	SetClassName("EntityPlayer");
	EnableCaps(CAP_NET_UPDATABLE);

	m_activated = false;

	m_controller = 0;
}

EntityPlayer::~EntityPlayer()
{
}

void EntityPlayer::Initialize(Client* client, InputManager* inputMgr)
{
	Assert(client);
	Assert(inputMgr);

	m_inputMgr = inputMgr;
}

void EntityPlayer::UpdateLogic(float dt)
{
}

void EntityPlayer::UpdateAnimation(float dt)
{
}

void EntityPlayer::Render()
{
}

void EntityPlayer::OnSceneAdd(Scene* scene)
{
	Assert(m_inputMgr);

	Entity::OnSceneAdd(scene);

	if (!m_client->m_clientSide)
		GetScene()->SetController(m_client, CONTROLLER_EXAMPLE);
}

void EntityPlayer::OnSceneRemove(Scene* scene)
{
	Entity::OnSceneRemove(scene);
}

void EntityPlayer::OnActivate()
{
	DB_TRACE("");

	m_activated = true;

	m_client->SetActionHandler(this);
}

void EntityPlayer::OnDeActivate()
{
	m_activated = false;
}

void EntityPlayer::OnAction(int actionID, float value)
{
}

void EntityPlayer::OnMessage(int message, int value)
{
}

float EntityPlayer::GetFOV() const
{
	return Calc::mPI / 2.0f;
}

Mat4x4 EntityPlayer::GetCamera() const
{
	return GetTransform();
}

Mat4x4 EntityPlayer::GetPerspective() const
{
	float fov = GetFOV();

	Mat4x4 result;

	result.MakePerspectiveLH(fov, GetScene()->GetRenderer()->GetAspect(), GetScene()->GetRenderer()->m_camNear, GetScene()->GetRenderer()->m_camFar);

	return result;
}

Vec3 EntityPlayer::GetCameraPosition() const
{
	return GetCamera().Mul(Vec4(0.0f, 0.0f, 0.0f, 1.0f)).XYZ();
}

Vec3 EntityPlayer::GetCameraOrientation() const
{
	return GetCamera().Mul(Vec4(0.0f, 0.0f, 1.0f, 0.0f)).XYZ();
}

void EntityPlayer::SetController(int id)
{
	Controller* controller = 0;

	for (size_t i = 0; i < m_controllers.size(); ++i)
		if (m_controllers[i]->GetID() == id)
			controller = m_controllers[i];

	m_controller = controller;
}
