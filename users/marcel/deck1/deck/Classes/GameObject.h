#pragma once

#include "deck_forward.h"
#include "GameObjectType.h"
#include "ScreenSide.h"

class GameObject
{
public:
	GameObject(Shape* shape, bool ownShape, int objectId, PrimType type);
	~GameObject();
	
	void Update(float dt);
	void Render();
	
	void Disappear(ScreenSide side);
	
	Shape* mShape;
	int mObjectId;
	PrimType mType;
	bool mIsDead;
};