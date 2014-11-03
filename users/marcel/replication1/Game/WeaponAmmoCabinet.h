#ifndef WEAPONAMMOCABINET_H
#define WEAPONAMMOCABINET_H
#pragma once

typedef void (*DelegateCB)(void* o, void* args[]);

class Delegate
{
public:
	inline Delegate()
	{
		m_o = 0;
		m_cb = 0;
	}

	inline void Assign(void* o, DelegateCB cb)
	{
		m_o = o;
		m_cb = cb;
	}

	inline bool IsSet()
	{
		return m_cb != 0;
	}

	inline void operator()(void* args[])
	{
		if (m_cb)
			m_cb(m_o, args);
	}

private:
	void* m_o;
	DelegateCB m_cb;
};

class WeaponAmmoCabinet
{
public:
	WeaponAmmoCabinet();

	void Initialize(int ammo, int clipSize, float loadTime, float loadClipTime);

	void Add(int ammo);
	void Reload();
	void Fire();

	void Update(float dt);

	Delegate OnFire;
	Delegate OnFireEmpty;
	Delegate OnReload;
	Delegate OnReloadFinish;

private:
	enum STATE
	{
		STATE_READY,
		STATE_FIRED,
		STATE_RELOAD
	};

	const static int CLIP_SIZE = 25;

	STATE m_state;
	int m_ammo;
	int m_clip;
	int m_clipSize;
	int m_refill;
	float m_loadTime;
	float m_load;
	float m_loadClipTime;
	float m_loadClip;
};

#endif
