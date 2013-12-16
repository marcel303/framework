#include <assert.h>
#include <math.h>
#include "bullet.h"
#include "enemy.h"
#include "eventmgr.h"
#include "game.h"
#include "input.h"
#include "player.h"
#include "powerup.h"
#include "render.h"
#include "sounds.h"
#include "StringEx.h"
#include "world.h"

#define SPAWN_DURATION 0.3f
#define HEALTH 100.0f
#define HEALTH_SPEED 60.0f
#define SPEED 150.0f
#define FIRE_RED_INTERVAL 3
#define FIRE_RED_DURATION 2.5f
#define FIRE_ORANGE_INTERVAL 1
#define FIRE_ORANGE_DURATION 2.5f
#define FIRE_SHOCKWAVE_DURATION 0.5f
#define FIRE_WAVE_DURATION 0.8f
#define COMBO_TIMEOUT 2.0f

Player::Player()
{
	mPosition.x = 40.0f;
	mPosition.y = 40.0f;

	mSpawnTimer = SPAWN_DURATION;

	mFireRedTimer = 0.0f;
	mFireRedCounter = 0;
	mFireRedDirection = 0;
	mFireOrangeTimer = 0.0f;
	mFireOrangeCounter = 0;
	mFireShockwaveTimer = 0.0f;

	mComboTimer = 0.0f;
	mComboCounter1 = 0;
	mComboCounter2 = 0;
	
	mHealth = HEALTH;
	mHealthRestore = true;
}

Player::~Player()
{
}

void Player::Update(float dt)
{
	mSpeed.SetZero();
	
	if (gKeyboard.Get(KeyCode_Left))
		mSpeed.x -= SPEED * dt;
	if (gKeyboard.Get(KeyCode_Right))
		mSpeed.x += SPEED * dt;
	if (gKeyboard.Get(KeyCode_Up))
		mSpeed.y -= SPEED * dt;
	if (gKeyboard.Get(KeyCode_Down))
		mSpeed.y += SPEED * dt;

	mSpeed += gKeyboard.TiltGet() * SPEED * 3.0f * dt;
	
	if (!gGame->IsPlaying_get())
	{
		static int frame = 0;
		if ((frame % 10) == 0)
		{
			mPosition.x = Random(0.0f, gWorld->mSx);
			mPosition.y = Random(0.0f, gWorld->mSy);
		}
		frame++;
	}
	else
	{
		if (mSpawnTimer <= 0.0f)
		{
#ifdef DEBUG
#warning
			static int frame = 0;
			if ((frame % 15) == 0)
			{
				mPosition.x = Random(0.0f, gWorld->mSx);
				mPosition.y = Random(0.0f, gWorld->mSy);
			}
			frame++;
#else
			mPosition.x += mSpeed.x;
			mPosition.y += mSpeed.y;
#endif
		}
	}
	
	while (mPosition.x < 0.0f)
		mPosition.x += gWorld->mSx;
	while (mPosition.y < 0.0f)
		mPosition.y += gWorld->mSy;
	while (mPosition.x > gWorld->mSx)
		mPosition.x -= gWorld->mSx;
	while (mPosition.y > gWorld->mSy)
		mPosition.y -= gWorld->mSy;

	if (mSpawnTimer > 0.0f)
	{
		mSpawnTimer -= dt;
	}

	if (mFireRedTimer > 0.0f)
	{
		if ((mFireRedCounter % FIRE_RED_INTERVAL) == 0)
		{
			for (int i = 0; i < 4; ++i)
			{
				if (i != mFireRedDirection)
					continue;

				float angle = (float)M_PI / 2.0f * i;
				Vec2F tangent = Vec2F::FromAngle(angle + (float)M_PI / 2.0f);
				//for (int j = -2; j <= +2; ++j)
				for (int j = -1; j <= +1; ++j)
				{
					Bullet* bullet = gWorld->AllocateBullet();
					bullet->Make_Red(
						mPosition.x + tangent.x * j * 8.0f,
						mPosition.y + tangent.y * j * 8.0f,
						angle);
				}
			}

			mFireRedDirection++;
			mFireRedDirection %= 4;
		}

		mFireRedCounter++;

		mFireRedTimer -= dt;
	}

	if (mFireOrangeTimer > 0.0f)
	{
		if ((mFireOrangeCounter % FIRE_ORANGE_INTERVAL) == 0)
		{
			Bullet* bullet = gWorld->AllocateBullet();
			bullet->Make_Orange(mPosition.x, mPosition.y);
		}

		mFireOrangeCounter++;

		mFireOrangeTimer -= dt;
	}
	if (mFireShockwaveTimer > 0.0f)
	{
		mFireShockwaveTimer -= dt;
	}
	if (mFireWaveTimer > 0.0f)
	{
		mFireWaveTimer -= dt;

		if (mFireWaveTimer <= 0.0f)
		{
			int n = 5;
			float angle = mSpeed.ToAngle();
			float angleStep = 15.0f / n * (float)M_PI / 180.0f;
			for (int i = -n; i <= +n; ++i)
			{
				Bullet* bullet = gWorld->AllocateBullet();
				bullet->Make_Intersector(mPosition.x, mPosition.y, angle + angleStep * i);
			}
		}
	}
	
	if (mComboTimer > 0.0f)
	{
		mComboTimer -= dt;
		
		if (mComboTimer <= 0.0f)
		{
			CommitCombo();
		}
	}

	if (mHealth < HEALTH && mHealthRestore)
	{
		mHealth += HEALTH_SPEED * dt;

		if (mHealth > HEALTH)
			mHealth = HEALTH;
	}

	mHealthRestored = mHealthRestore;

	mHealthRestore = true;
}

void Player::Render()
{
	if (mSpawnTimer > 0.0f)
	{
		float t = mSpawnTimer / SPAWN_DURATION;

		gRender->Arc(mPosition.x, mPosition.y, 0.0f, M_PI * 2.0f, 7.0f + (int)(t * 5.0f) * 2.0f, Color(1.0f, 1.0f, 1.0f));
	}

	gRender->Circle(mPosition.x, mPosition.y, 5.0f, Color(1.0f, 1.0f, 1.0f));

	if (mFireShockwaveTimer > 0.0f)
	{
		float t = mFireShockwaveTimer / FIRE_SHOCKWAVE_DURATION * 1.1f;

		if (t > 1.0f)
			t = 1.0f;

		gRender->Quad(0.0f, 0.0f, gWorld->mSx, gWorld->mSy, Color(0.0f, 0.0f, 1.0f, t));
	}

	if (mHealth < HEALTH)
	{
		float t = (HEALTH - mHealth) / HEALTH;

		gRender->Quad(0.0f, 0.0f, gWorld->mSx, gWorld->mSy, Color(1.0f, 0.0f, 0.0f, t));
	}
}

bool Player::CollidesWith(const Vec2F& position) const
{
	return mPosition.DistanceTo(position) < 20.0f;
}

void Player::HandlePowerup(Powerup* powerup)
{
	switch (powerup->PowerupType_get())
	{
	case PowerupType_Red:
		SoundPlay(Sound_Vulcan2);
		mFireRedTimer = FIRE_RED_DURATION;
		break;
	case PowerupType_Missiles:
		SoundPlay(Sound_Missile2);
		for (int i = 0; i < 50; ++i)
		{
			Bullet* bullet = gWorld->AllocateBullet();
			bullet->Make_Missile(mPosition.x, mPosition.y, 0);
		}
		break;
	case PowerupType_Swarm:
		{
			float angle = Random(0.0f, (float)M_PI * 2.0f);
			float angleStep = M_PI * 2.0f / 3.0f;
			for (int i = 0; i < 3; ++i)
			{
				Bullet* bullet = gWorld->AllocateBullet();
				bullet->Make_Swarm(mPosition.x, mPosition.y, angle + angleStep * i);
			}
			break;
		}
	case PowerupType_Orange:
		SoundPlay(Sound_Vulcan2);
		mFireOrangeTimer = FIRE_ORANGE_DURATION;
		break;
	case PowerupType_Shockwave:
		SoundPlay(Sound_Shockwave2);
		mFireShockwaveTimer = FIRE_SHOCKWAVE_DURATION;
		for (std::list<Enemy*>::iterator i = gWorld->mEnemyList.begin(); i != gWorld->mEnemyList.end(); ++i)
		{
			Enemy* enemy = *i;
			enemy->Kill();
		}
		break;
	case PowerupType_Wave:
		mFireWaveTimer = FIRE_WAVE_DURATION;
		break;
	}

	mComboTimer = COMBO_TIMEOUT;
	mComboCounter1++;
}

void Player::HandleKill()
{
	mComboCounter2++;
}

void Player::Damage(float amount)
{
	if (!gGame->IsPlaying_get())
		return;

	if (mHealthRestored)
	{
		SoundPlay(Sound_Damage2);
	}

	mHealth -= amount;
	mHealthRestore = false;

	if (mHealth <= 0.0f)
	{
		mHealth = 0.0f;

		gEventMgr->Push(Event(EventType_Die));
		
		gGame->End();
	}
}

void Player::CommitCombo()
{
	int combo = mComboCounter1 * mComboCounter2;

	if (combo > 0)
	{
		gGame->Score_add(combo);
		
		gEventMgr->Push(Event(EventType_Combo, mComboCounter1));
	}
	
	mComboCounter1 = 0;
	mComboCounter2 = 0;
}

std::string Player::ComboString_get()
{
	return String::Format("%d x %d", mComboCounter1, mComboCounter2);
}

int Player::ComboCountA_get() const
{
	return mComboCounter1;
}

int Player::ComboCountB_get() const
{
	return mComboCounter2;
}
