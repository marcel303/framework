#include "Calc.h"
#include "EntityBrick.h"
#include "EntityBrickSpawn.h"
#include "Timer.h"

EntityBrickSpawn::EntityBrickSpawn() : Entity()
{
	SetClassName("BrickSpawn");

	m_timer.Initialize(&g_TimerRT);
	m_timer.SetInterval(1.0);
	m_timer.Start();
}

EntityBrickSpawn::~EntityBrickSpawn()
{
}

void EntityBrickSpawn::UpdateLogic(float dt)
{
	int todo = 50 - GetScene()->FindEntitiesByClassName("Brick").size();

	while (m_timer.ReadTick() && todo > 0) // todo : should updated timer based on dt. create game timer?
	{
		todo--;

		float x = Calc::Random(-150.0f, +150.0f);
		float z = Calc::Random(-150.0f, +150.0f);
		float size = 5.0f + Calc::Random(0.0f, 20.0f);

		EntityBrick* brick = new EntityBrick(Vec3(x, 0.0f, z), Vec3(15.0f, size, 15.0f), false);
		brick->PostCreate(); // fixme, use factory.
		GetScene()->AddEntityQueued(ShEntity(brick));
	}
}