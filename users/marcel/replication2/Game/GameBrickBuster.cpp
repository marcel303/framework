#include "GameBrickBuster.h"

#include "Entity.h"
#include "EntityBrickSpawn.h"
#include "EntityFloor.h"
#include "Player.h"

GameBrickBuster::GameBrickBuster()
	: Game()
{
}

GameConfig GameBrickBuster::CreateConfig()
{
	GameConfig cfg;

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
			serverScene->AddEntity(ShEntity(spawn));
		}
		{
			EntityFloor* floor = new EntityFloor();
			floor->Initialize(Vec3(0.0f, 50.0f, 0.0f), Vec3(size, 10.0f, size), -1);
			serverScene->AddEntity(ShEntity(floor));
		}
		{
			EntityFloor* floor = new EntityFloor();
			floor->Initialize(Vec3(0.0f, 0.0f, 0.0f), Vec3(size, 10.0f, size), +1);
			serverScene->AddEntity(ShEntity(floor));
		}
		{
			EntityFloor* floor = new EntityFloor();
			floor->Initialize(Vec3(-size / 2.0f, 25.0f, 0.0f), Vec3(10.0f, 50.0f, size), +1);
			serverScene->AddEntity(ShEntity(floor));
		}
		{
			EntityFloor* floor = new EntityFloor();
			floor->Initialize(Vec3(+size / 2.0f, 25.0f, 0.0f), Vec3(10.0f, 50.0f, size), +1);
			serverScene->AddEntity(ShEntity(floor));
		}
		{
			EntityFloor* floor = new EntityFloor();
			floor->Initialize(Vec3(0.0f, 25.0f, -size / 2.0f), Vec3(size, 50.0f, 10.0f), +1);
			serverScene->AddEntity(ShEntity(floor));
		}
		{
			EntityFloor* floor = new EntityFloor();
			floor->Initialize(Vec3(0.0f, 25.0f, +size / 2.0f), Vec3(size, 50.0f, 10.0f), +1);
			serverScene->AddEntity(ShEntity(floor));
		}
	}
}

void GameBrickBuster::HandlePlayerConnect(Client* client)
{
	// TODO: Callback for new player creation.
	Player * player = new Player(client->m_channel->m_destinationId);
	player->Initialize(client, GetEngine()->m_inputMgr);

	GetEngine()->BindClientToEntity(client, ShEntity(player));
}