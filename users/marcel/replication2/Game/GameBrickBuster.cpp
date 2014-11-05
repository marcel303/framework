#include "GameBrickBuster.h"

#include "Entity.h"
#include "EntityBrickSpawn.h" // FIXME
#include "EntityFloor.h" // FIXME
#include "Player.h"

GameBrickBuster::GameBrickBuster() : Game()
{
}

GameConfig GameBrickBuster::CreateConfig()
{
	GameConfig cfg;

	//cfg.Initialize(GameConfig::GRAPHICS_D3D, 100, 0);
	cfg.Initialize(GameConfig::GRAPHICS_OPENGL, 100, 0);

	return cfg;
}

void GameBrickBuster::HandleSceneLoadBegin()
{
	Engine* engine = GetEngine();

	Scene* serverScene = engine->m_serverScene;

	// TODO: Remove check, always server side.
	if (engine->m_role & Engine::ROLE_SERVER)
	{
		// TODO: Load scene.
		const float size = 300.0f;

		{
			EntityBrickSpawn* spawn = new EntityBrickSpawn();
			spawn->PostCreate(); // fixme, use factory.
			serverScene->AddEntity(ShEntity(spawn));
		}
		{
			EntityFloor* floor = new EntityFloor(Vec3(0.0f, 50.0f, 0.0f), Vec3(size, 10.0f, size), -1);
			floor->PostCreate(); // fixme, use factory.
			serverScene->AddEntity(ShEntity(floor));
		}
		{
			EntityFloor* floor = new EntityFloor(Vec3(0.0f, 0.0f, 0.0f), Vec3(size, 10.0f, size), +1);
			floor->PostCreate(); // fixme, use factory.
			serverScene->AddEntity(ShEntity(floor));
		}
		{
			EntityFloor* floor = new EntityFloor(Vec3(-size / 2.0f, 25.0f, 0.0f), Vec3(10.0f, 50.0f, size), +1);
			floor->PostCreate(); // fixme, use factory.
			serverScene->AddEntity(ShEntity(floor));
		}
		{
			EntityFloor* floor = new EntityFloor(Vec3(+size / 2.0f, 25.0f, 0.0f), Vec3(10.0f, 50.0f, size), +1);
			floor->PostCreate(); // fixme, use factory.
			serverScene->AddEntity(ShEntity(floor));
		}
		{
			EntityFloor* floor = new EntityFloor(Vec3(0.0f, 25.0f, -size / 2.0f), Vec3(size, 50.0f, 10.0f), +1);
			floor->PostCreate(); // fixme, use factory.
			serverScene->AddEntity(ShEntity(floor));
		}
		{
			EntityFloor* floor = new EntityFloor(Vec3(0.0f, 25.0f, +size / 2.0f), Vec3(size, 50.0f, 10.0f), +1);
			floor->PostCreate(); // fixme, use factory.
			serverScene->AddEntity(ShEntity(floor));
		}
	}
}

void GameBrickBuster::HandlePlayerConnect(Client* client)
{
	// TODO: Callback for new player creation.
	ShEntity player = ShEntity(new Player(client, GetEngine()->m_inputMgr));
	player->PostCreate();

	GetEngine()->BindClientToEntity(client, player);
}