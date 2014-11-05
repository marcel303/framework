#include "Calc.h"
#include "EntityBrick.h"
#include "Player.h"
#include "ProcTexcoordMatrix2DAutoAlign.h"
#include "Renderer.h"
#include "ShapeBuilder.h"
#include "ResMgr.h"
#include "WeaponDefault.h"

Player::Player(Client* client, InputManager* inputMgr) : EntityPlayer(client, inputMgr), m_controllerExample(client),
	m_weaponLink("weapon", this), m_fort("fort", this)
{
	SetClassName("Player");
	EnableCaps(CAP_DYNAMIC_PHYSICS);

	m_player_NS = new Player_NS(this);

	m_control = new PlayerControl(this);
}

Player::~Player()
{
	delete m_player_NS;
	m_player_NS = 0;

	delete m_control;
	m_control = 0;
}

void Player::PostCreate()
{
	EntityPlayer::PostCreate();

	// Shape.
	m_phyObject.Initialize(0, Vec3(0.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 0.0f), true, true, this);
	//m_phyObject.m_velocityDamp = Vec3(0.001f, 0.1f, 0.001f);
	m_phyObject.AddGeometry(CD::ShObject(new CD::Cube(Vec3(-3.0f, 0.0f, -3.0f), Vec3(+3.0f, +8.0f, +3.0f))));

	//m_color = makecol(255, 255, 255);

	// Visuals.
	ShapeBuilder sb;
	sb.PushTranslation(Vec3(0.0f, 2.0f, 0.0f));
	sb.PushScaling(Vec3(3.0f, 4.0f, 3.0f));
	Mesh temp;
	sb.CreateCilinder(&g_alloc, 10, true, ShapeBuilder::AXIS_Y, temp, FVF_XYZ | FVF_TEX1 | FVF_NORMAL);
	ProcTexcoordMatrix2DAutoAlign procTex;
	Mat4x4 mat;
	mat.MakeScaling(0.1f, 0.1f, 0.1f);
	procTex.SetMatrix(mat);
	sb.CalculateNormals(temp);
	sb.CalculateTexcoords(temp, 0, &procTex);
	sb.ConvertToIndexed(&g_alloc, temp, m_mesh);
	sb.Pop();
	sb.Pop();

	// Sounds.
	m_sndSrc = ShSndSrc(new ResSndSrc());

	m_sndJump = RESMGR.GetSnd("data/sounds/jump.ogg");
	m_sndHurt = RESMGR.GetSnd("data/sounds/hurt.ogg");

	// Controllers.
	m_controllers.push_back(&m_controllerExample);
}

Mat4x4 Player::GetTransform() const
{
	return m_control->GetTransform(m_phyObject.m_position);
}

void Player::UpdateLogic(float dt)
{
	Entity::UpdateLogic(dt);

	if (m_control->m_controls.m_fire)
		m_weapon->FirePrimary();

	float distance;
	ShEntity hit = m_scene->CastRay(this, GetCameraPosition(), Vec3(0.0f, -1.0f, 0.0f), 1000.0f, &distance);

	// Fortify brick beneath player.
	{
		Vec3 orientation(0.0f, -1.0f, 0.0f);

		ShEntity hit = m_scene->CastRay(this, GetCameraPosition(), orientation, 1000.0f, 0);

		if (hit.get() && hit.get()->m_className == "Brick")
			SetFort(hit);
		else
			SetFort(ShEntity((Entity*)0));
	}
}

void Player::UpdateAnimation(float dt)
{
	Entity::UpdateAnimation(dt);

	if (m_controller)
		m_controller->Update(m_client->m_channel);

	m_control->Animate(&m_phyObject, dt);
}

void Player::Render()
{
	if (!m_activated)
	{
		Renderer::I().RenderMesh(m_mesh);
	}
}

Mat4x4 Player::GetCamera() const
{
	Vec3 height = Vec3(0.0f, 5.0f, 0.0f);

	return m_control->GetTransform(m_phyObject.m_position + height);
}

float Player::GetFOV() const
{
	if (m_weaponLink.Valid())
		return ((const Weapon*)m_weaponLink.Get())->GetFOV();
	else
		return Calc::mPI / 2.0f;
}

void Player::OnSceneAdd(Scene* scene)
{
	EntityPlayer::OnSceneAdd(scene);

	if (!m_client->m_clientSide)
	{
		m_weapon = ShWeapon(new WeaponDefault(this));

		scene->AddEntity(m_weapon);

		m_weaponLink = m_weapon;
	}
}

void Player::OnSceneRemove(Scene* scene)
{
	EntityPlayer::OnSceneRemove(scene);

	if (!m_client->m_clientSide)
	{
		scene->RemoveEntity(m_weapon);
	}
}

void Player::OnAction(int actionID, float value)
{
	switch (actionID)
	{
	case ControllerExample::ACTION_MOVE_FORWARD:
		m_control->MoveForward(value != 0.0f);
		break;
	case ControllerExample::ACTION_MOVE_BACK:
		m_control->MoveBack(value != 0.0f);
		break;
	case ControllerExample::ACTION_STRAFE_LEFT:
		m_control->StrafeLeft(value != 0.0f);
		break;
	case ControllerExample::ACTION_STRAFE_RIGHT:
		m_control->StrafeRight(value != 0.0f);
		break;
	case ControllerExample::ACTION_JUMP:
		// FIXME.
		m_control->m_controls.m_jump = value != 0.0f;
		if (m_control->m_controls.m_jump)
		{
			float distance;
			ShEntity hit = m_scene->CastRay(this, GetCameraPosition(), Vec3(0.0f, -1.0f, 0.0f), 1000.0f, &distance);
			if (hit.get() && distance <= 8.0f)
			{
				if (!m_client->m_clientSide)
					ReplicatedMessage(MSG_PLAYSOUND, SND_JUMP);
				m_phyObject.m_force[1] += 200.0f * 50.0f;
			}
		}
		break;
	case ControllerExample::ACTION_ROTATE_H:
		m_control->RotateH(+value / 400.0f);
		break;
	case ControllerExample::ACTION_ROTATE_V:
		m_control->RotateV(-value / 400.0f);
		break;
	case ControllerExample::ACTION_FIRE:
		m_control->m_controls.m_fire = value != 0.0f;
		//m_parameters["firing"]->Invalidate(); // todo
		break;
	case ControllerExample::ACTION_ZOOM:
		if (!m_client->m_clientSide)
		{
			if (value)
				if (m_weaponLink.Valid())
					((Weapon*)m_weaponLink.Get())->Zoom();
		}
		break;
	}
}

void Player::OnMessage(int message, int value)
{
	//printf("Message: %d, %d.\n", message, value);

	// TODO: Make MSG_PLAYSOUND common message.
	// TODO: Add AddSound somewhere.
	if (message == MSG_PLAYSOUND)
	{
		SoundDevice* sfx = Renderer::I().GetSoundDevice();

		if (value == SND_JUMP) sfx->PlaySound(m_sndSrc.get(), m_sndJump.get(), false);
		if (value == SND_HURT) sfx->PlaySound(m_sndSrc.get(), m_sndHurt.get(),  false);

		m_sndSrc->SetPosition(GetCameraPosition());
		m_sndSrc->SetVelocity(m_phyObject.m_velocity);
	}
}

void Player::SetFort(ShEntity fort)
{
	if (m_fort.Get() == fort.get())
		return;

	if (m_fort.Valid())
		m_fort.GetT()->Fort(-1);

	m_fort = fort;

	if (m_fort.Valid())
		m_fort.GetT()->Fort(+1);
}
