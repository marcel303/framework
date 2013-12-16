#pragma once

#include "GoBlackHole.h"
#include "IObject.h"
#include "Types.h"
#include "PowerSink.h"
#include "SelectionMap.h"

class Ship : public IObject // Enemy ship / boss attack fleet.
{
public:
	Ship(Map* map);

	void Spawn(Vec2F pos);

	virtual void Update(Map* map);
	virtual void Render(BITMAP* buffer);
	virtual void RenderSB(SelectionBuffer* sb);

	void Hit(int hitPoints);

	static void HandleBlackHole(void* self, BlackHole* hole);

	Vec2F Pos;
	Vec2F Speed;
	Vec2F Force;
	int HitPoints;

	PowerSink m_PowerSink;

	//CD_TYPE m_SelectionId;
	SelectionId m_SelectionId;
};
