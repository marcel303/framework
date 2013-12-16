#pragma once

#include "libgg_forward.h"
#include "Types.h"

class EnemyPathNode
{
public:
	EnemyPathNode();
	
	Vec2F position;
	float distance;
};

class EnemyPath
{
public:
	EnemyPath();
	~EnemyPath();
	
	void Load(Stream* stream);
	void Save(Stream* stream);
	
	void Update(float dt);
	void Render();
	
	int NodeCount_get() const;
	EnemyPathNode* Node_get(int index);
	
	float CalculateDistance(Vec2F location);
	
private:
	EnemyPathNode* mNodes;
	int mNodeCount;
};
