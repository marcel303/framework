#include "Calc.h"
#include "Player.h"
#include "Weapon.h"

DEFINE_ENTITY(Weapon, Weapon);

Weapon::Weapon(Player* owner)
	: Entity()
	, m_ownerLink("owner", this)
{
	SetClassName("Weapon");

	if (owner)
	{
		m_ownerLink = owner->GetID();
	}
}

Weapon::~Weapon()
{
}

Player* Weapon::GetOwner()
{
	return static_cast<Player*>(m_ownerLink.Get());
}

float Weapon::GetFOV() const
{
	return Calc::mPI / 2.0f;
}

void Weapon::FirePrimary()
{
}

void Weapon::FireSecondary()
{
}

void Weapon::Zoom()
{
}
