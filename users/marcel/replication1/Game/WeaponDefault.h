#ifndef WEAPONDEFAULT_H
#define WEAPONDEFAULT_H
#pragma once

#include "SoundDevice.h"
#include "Weapon.h"
#include "WeaponAmmoCabinet.h"

// Fires ammo with anti brick damage~
class WeaponDefault : public Weapon
{
public:
	WeaponDefault(Player* owner);

	virtual float GetFOV() const;

	virtual void FirePrimary();
	virtual void FireSecondary();
	virtual void Zoom();
	virtual void UpdateLogic(float dt);
	virtual void UpdateAnimation(float dt);

	virtual void OnMessage(int message, int value);

private:
	const static int MSG_PLAYSOUND = 0;
	const static int SND_FIRE = 0;
	const static int SND_HIT = 1;
	const static int SND_RELOAD = 2;
	const static int SND_ZOOM = 3;

	const static int CLIP_SIZE = 25;

	void HandleFire();
	void HandleReload();

	static void HandleFireCB(void* o, void* args[])
	{
		WeaponDefault* w = (WeaponDefault*)o;

		w->HandleFire();
	}

	static void HandleReloadCB(void* o, void* args[])
	{
		WeaponDefault* w = (WeaponDefault*)o;

		w->HandleReload();
	}

	WeaponAmmoCabinet m_cabinet1;

#if 0
	float m_reloadPrimary;
	float m_reloadPrimaryClip;
	int m_ammo;
	int m_clip;
#endif
	int8_t m_zoom;
	float m_currZoom;

	ShSndSrc m_sndSrc;

	ShSnd m_sndFire;
	ShSnd m_sndHit;
	ShSnd m_sndReload;
	ShSnd m_sndZoom;
};

#endif
