#include "WeaponAmmoCabinet.h"

WeaponAmmoCabinet::WeaponAmmoCabinet()
{
	m_state = STATE_READY;

	Initialize(0, 0, 0.0f, 0.0f);
}

void WeaponAmmoCabinet::Initialize(int ammo, int clipSize, float loadTime, float loadClipTime)
{
	m_ammo = ammo;
	m_clip = 0;
	m_clipSize = clipSize;
	m_refill = 0;
	m_loadTime = loadTime;
	m_load = 0.0f;
	m_loadClipTime = loadClipTime;
	m_loadClip = 0.0f;

	m_state = STATE_READY;
}

void WeaponAmmoCabinet::Add(int ammo)
{
	m_ammo += ammo;
}

void WeaponAmmoCabinet::Reload()
{
	if (m_state != STATE_RELOAD)
	{
		m_loadClip = m_loadClipTime;

		m_refill = m_clipSize;

		if (m_refill > m_ammo)
			m_refill = m_ammo;

		m_state = STATE_RELOAD;

		OnReload(0);
	}
}

void WeaponAmmoCabinet::Fire()
{
	if (m_state == STATE_READY)
	{
		if (m_clip > 0)
		{
			OnFire(0);

			m_clip--;

			m_load = m_loadTime;

			m_state = STATE_FIRED;

			if (m_clip == 0)
			{
				OnFireEmpty(0);

				Reload();
			}
		}
		else
		{
			//OnFireEmpty(0);

			Reload();
		}
	}
}

void WeaponAmmoCabinet::Update(float dt)
{
	if (m_state == STATE_READY)
	{
	}
	else if (m_state == STATE_FIRED)
	{
		m_load -= dt;

		if (m_load < 0.0f)
			m_load = 0.0f;

		if (m_load == 0.0f)
		{
			m_state = STATE_READY;
		}
	}
	else if (m_state == STATE_RELOAD)
	{
		m_loadClip -= dt;

		if (m_loadClip < 0.0f)
			m_loadClip = 0.0f;

		if (m_loadClip == 0.0f)
		{
			m_ammo -= m_refill;
			m_clip += m_refill;

			m_state = STATE_READY;
		}
	}
}
