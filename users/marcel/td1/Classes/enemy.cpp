#include "Calc.h"
#include "enemy.h"
#include "enemy_info.h"
#include "enemy_path.h"
#include "game.h"
#include "level.h"
#include "Log.h"
#include "particle.h"
#include "Render.h"
#include "sounds.h"
#include "SpriteGfx.h"

Enemy::Enemy()
{
	mType = EnemyType_Undefined;
	mFlags = 0;
	mId = gEnemyInfoMgr->Allocate();
	mHealth = 0.0f;
	mPath = 0;
	mPathNode = 0;
	mPathProgress = 0.0f;
	mPathSpeed = 0.0f;
	
	//
}


Enemy::~Enemy()
{
	LOG_DBG("dealloc: enemy", 0);
	
	gEnemyInfoMgr->Free(mId);
}

void Enemy::Make(EnemyDesc desc)
{
	mType = desc.type;
	mHealth = desc.health;
	mPathSpeed = desc.speed;
}

void Enemy::Attach(EnemyPath* path)
{
	mPath = path;
}

void Enemy::Update(float dt)
{
	EnemyPathNode* node = mPath->Node_get(mPathNode);
	
	if (node)
	{
		if (mPathProgress >= node->distance)
		{
			mPathProgress -= node->distance;
			
			if (HasNextPathNode())
			{
				NextPathNode();
			}
			else
			{
				ExecuteEscape();
			}
		}
		
		float speed = mPathSpeed;
		
		if (mFlags & EnemyFlag_Slow)
		{
			mFlags -= EnemyFlag_Slow;

			speed *= 0.5f;
		}
		
		mPathProgress += speed * dt;
	}
	else
	{
		LOG_DBG("node does not exist: %d", mPathNode);
	}
	
	gEnemyInfoMgr->Update(mId, Position_get());
}

void Enemy::Render()
{
	Vec2F position = Position_get();
	
	gRender->Circle(position[0], position[1], 2.0f, Color(1.0f, 1.0f, 1.0f));
}

int Enemy::Flags_get() const
{
	return mFlags;
}

Vec2F Enemy::Position_get()
{
	EnemyPathNode* node1 = mPath->Node_get(mPathNode + 0);
	EnemyPathNode* node2 = mPath->Node_get(mPathNode + 1);
	
	Assert(node1);
	Assert(node2);
	
	return node1->position.LerpTo(node2->position, mPathProgress / node1->distance);
}

int Enemy::Id_get() const
{
	return mId;
}

void Enemy::HandleDamage(float amount)
{
	if (mFlags & EnemyFlag_Dead)
		return;
	
	mHealth -= amount;
	
	if (mHealth <= 0.0f)
	{
		ExecuteDie();
	}
}

void Enemy::HandleSlow()
{
	mFlags |= EnemyFlag_Slow;
}

void Enemy::ExecuteEscape()
{
	mFlags |= EnemyFlag_Escaped | EnemyFlag_Dead;
}

void Enemy::ExecuteDie()
{
	mHealth = 0.0f;
	
	mFlags |= EnemyFlag_Dead;
	
	gGame->Level_get()->BuildMoney_increase(10.0f); // todo: combo or smt
	
	for (int i = 0; i < 20; ++i)
	{
		Particle* p = gParticleMgr->Allocate();
		
		p->Setup(Position_get(), Vec2F::FromAngle(Calc::Random(Calc::m2PI)) * 50.0f, 1.0f, 8.0f, 2.0f, SpriteColor_Make(255, 255, 255, 255));
	}
	
	SoundPlay(Sound_Enemy_Die);
}

bool Enemy::HasNextPathNode()
{
	return mPathNode + 2 < mPath->NodeCount_get();
}

void Enemy::NextPathNode()
{
	mPathNode++;
}
