#pragma once

#include "td_forward.h"
#include "Types.h"

enum EnemyType
{
	EnemyType_Undefined,
	EnemyType_A,
	EnemyType_B,
};

enum EnemyFlag
{
	EnemyFlag_Dead = 1 << 0,
	EnemyFlag_Escaped = 1 << 1,
	EnemyFlag_Slow = 1 << 2
};

class EnemyDesc
{
public:
	EnemyType type;
	float health;
	float speed;
	float reward;
};

class Enemy
{
public:
	Enemy();
	~Enemy();
	
	void Make(EnemyDesc desc);
	void Attach(EnemyPath* path);
	
	void Update(float dt);
	void Render();
	
	int Flags_get() const;
	Vec2F Position_get();
	int Id_get() const;
	
	void HandleDamage(float amount);
	void HandleSlow();
	
private:
	void ExecuteEscape();
	void ExecuteDie();
	
	bool HasNextPathNode();
	void NextPathNode();
	
	EnemyType mType;
	int mFlags;
	int mId;
	
	// life
	float mHealth;
	
	// movement
	EnemyPath* mPath;
	int mPathNode;
	float mPathProgress;
	float mPathSpeed;
};
