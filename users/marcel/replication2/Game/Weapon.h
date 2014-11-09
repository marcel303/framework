#ifndef WEAPON_H
#define WEAPON_H
#pragma once

#include "Entity.h"
#include "SharedPtr.h"

class Player;

class Weapon : public Entity
{
public:
	Weapon(Player* owner = 0);
	virtual ~Weapon();

	Player* GetOwner();
	virtual float GetFOV() const;

	virtual void FirePrimary();
	virtual void FireSecondary();
	virtual void Zoom();

private:
	EntityLink<Player> m_ownerLink;
};

typedef SharedPtr<Weapon> ShWeapon;

#endif
