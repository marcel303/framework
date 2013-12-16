#pragma once

#include <string>
#include "forward.h"
#include "types2.h"

class Player
{
public:
	Player();
	~Player();

	void Update(float dt);
	void Render();

	bool CollidesWith(const Vec2F& position) const;

	void HandlePowerup(Powerup* powerup);
	void HandleKill();

	void Damage(float damage);
	
	void CommitCombo();
	
	std::string ComboString_get();
	int ComboCountA_get() const;
	int ComboCountB_get() const;

private:
	Vec2F mPosition;
	Vec2F mSpeed;

	float mSpawnTimer;

	float mFireRedTimer;
	int mFireRedCounter;
	int mFireRedDirection;

	float mFireOrangeTimer;
	int mFireOrangeCounter;

	float mFireShockwaveTimer;

	float mFireWaveTimer;
	
	float mComboTimer;
	int mComboCounter1;
	int mComboCounter2;

	float mHealth;
	bool mHealthRestore;
	int mHealthRestored;
};
