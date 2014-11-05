#include "Game.h"

GameConfig::GameConfig()
{
	//Initialize(GRAPHICS_D3D, 100, 0);
	Initialize(GRAPHICS_OPENGL, 100, 0);
}

void GameConfig::Initialize(GRAPHICS_DEVICE gfx, int logicHz, int fpsLimit)
{
	m_gfx = gfx;
	m_logicHz = logicHz;
	m_fpsLimit = fpsLimit;
}

Game::Game()
{
	m_engine = 0;

	// TODO: Move..
	m_cfg = CreateConfig();
}

Game::~Game()
{
}

GameConfig Game::CreateConfig()
{
	GameConfig cfg;

	return cfg;
}

void Game::HandleInitialize(Engine* engine)
{
	m_engine = engine;
}

void Game::HandleShutdown()
{
}

void Game::HandleSceneLoadBegin()
{
}

void Game::HandleSceneLoadEnd()
{
}

void Game::HandleSceneUnLoadBegin()
{
}

void Game::HandleSceneUnLoadEnd()
{
}

void Game::HandleGameStart()
{
}

void Game::HandleGameEnd()
{
}

void Game::HandlePlayerConnect(Client* client)
{
}

void Game::HandleUpdateServer(float dt)
{
}

void Game::HandleUpdateClient(float dt)
{
}

void Game::HandleRender()
{
}

const GameConfig& Game::GetConfig()
{
	return m_cfg;
}

Engine* Game::GetEngine()
{
	return m_engine;
}
