#pragma once

#include "GoBlackHole.h"
#include "GoGun.h"
#include "GoShip.h"
#include "GoSun.h"

#include "Map.h"
#include "Renderer.h"

class Boss : public IObject // Boss / It's huge, and has a serious appetite for energy.
{
public:
	Boss(Map* map) : IObject(ObjectType_Boss, map)
	{
	}

	virtual void Update(Map* map)
	{
	}

	virtual void Render(BITMAP* buffer)
	{
	}
};
