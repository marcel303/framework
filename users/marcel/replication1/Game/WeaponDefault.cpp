#include "Calc.h"
#include "EntityBrick.h"
#include "EntityParticles.h"
#include "Player.h"
#include "Renderer.h"
#include "ResMgr.h"
#include "WeaponDefault.h"

WeaponDefault::WeaponDefault(Player* owner) : Weapon(owner)
{
	SetClassName("WeaponDefault");
	AddParameter(Parameter(PARAM_INT8, "zoom", REP_ONCHANGE, COMPRESS_NONE, &m_zoom));
	
	m_sndSrc = ShSndSrc(new ResSndSrc());

	m_sndFire   = RESMGR.GetSnd("sounds/fire.ogg"  );
	m_sndHit    = RESMGR.GetSnd("sounds/hit.ogg"   );
	m_sndReload = RESMGR.GetSnd("sounds/reload.ogg");
	m_sndZoom   = RESMGR.GetSnd("sounds/zoom.ogg"  );

	//m_cabinet1.Initialize(1000000, CLIP_SIZE, 0.075f, 2.25f);
	m_cabinet1.Initialize(1000000, 1000000, 0.075f, 0.f);
	m_cabinet1.OnFire.Assign(this, HandleFireCB);
	m_cabinet1.OnReload.Assign(this, HandleReloadCB);

	m_zoom = 0;
	m_currZoom = m_zoom;
}

float WeaponDefault::GetFOV() const
{
	return Calc::mPI / (2.0f + m_currZoom * m_currZoom * 2.0f);
}

void WeaponDefault::HandleFire()
{
	ReplicatedMessage(MSG_PLAYSOUND, SND_FIRE);
	Vec3 position = GetOwner()->GetCameraPosition();
	Vec3 orientation = GetOwner()->GetCameraOrientation();

	if (GetOwner()->m_className == "Player")
		((Player*)GetOwner())->m_control.m_rotationX += 0.025f + Calc::Random(0.0f, 0.025f);

	float distance;
	ShEntity hit = GetOwner()->m_scene->CastRay(GetOwner(), position, orientation, 1000.0f, &distance);
	if (hit.get())
	{
		Vec3 hitpos = position + orientation * distance;

		if (hit->m_className == "Player")  // fixme: Or any other?
		{
			Player* player = (Player*)hit.get();

			player->m_control.m_rotationY = 0.0f;
			player->m_phyObject.m_force += player->GetCameraOrientation() * 100.0f;

			ReplicatedMessage(MSG_PLAYSOUND, SND_HIT);
		}
		if (hit->m_className == "Brick")
		{
			EntityBrick* brick = (EntityBrick*)hit.get();
			if (!brick->IsIndestructible())
			{
				brick->Damage(20);
				ReplicatedMessage(MSG_PLAYSOUND, SND_HIT);

				// FIXME: Shld rlly send replicated message instead of abusing replication.
				EntityParticles* particles = new EntityParticles(hitpos, 1, 0.5f);
				particles->PostCreate();
				m_scene->AddEntity(ShEntity(particles));
			}
		}
	}
}

void WeaponDefault::HandleReload()
{
	ReplicatedMessage(MSG_PLAYSOUND, SND_RELOAD);
}

void WeaponDefault::FirePrimary()
{
	m_cabinet1.Fire();
}

void WeaponDefault::FireSecondary()
{
}

void WeaponDefault::Zoom()
{
	m_zoom = (m_zoom + 1) % 3;
	m_parameters["zoom"]->Invalidate();
	ReplicatedMessage(MSG_PLAYSOUND, SND_ZOOM);
}

void WeaponDefault::UpdateLogic(float dt)
{
	Weapon::UpdateLogic(dt);

	m_cabinet1.Update(dt);
}

void WeaponDefault::UpdateAnimation(float dt)
{
	Weapon::UpdateAnimation(dt);

	float zoomDiff = m_zoom - m_currZoom;
	float zoomUpdate = zoomDiff * dt * 4.0f;
	if (fabs(zoomUpdate) > fabs(zoomDiff))
		zoomUpdate = zoomDiff;
	m_currZoom += zoomUpdate;
}

void WeaponDefault::OnMessage(int message, int value)
{
	// TODO: Make MSG_PLAYSOUND common message.
	// TODO: Add AddSound somewhere.
	if (message == MSG_PLAYSOUND)
	{
		Player* player = GetOwner();

		if (player)
		{
			SoundDevice* sfx = Renderer::I().GetSoundDevice();

			if (value == SND_FIRE)   sfx->PlaySound(m_sndSrc.get(), m_sndFire.get(),    false);
			if (value == SND_HIT)    sfx->PlaySound(m_sndSrc.get(), m_sndHit.get(),     false);
			if (value == SND_RELOAD) sfx->PlaySound(m_sndSrc.get(), m_sndReload.get(),  false);
			if (value == SND_ZOOM)   sfx->PlaySound(m_sndSrc.get(), m_sndZoom.get(),    false);

			m_sndSrc->SetPosition(player->GetCameraPosition() + player->GetCameraOrientation() * (value == SND_HIT ? 100.0f : 10.0f));
			m_sndSrc->SetVelocity(player->m_phyObject.m_velocity);
		}
	}
}
