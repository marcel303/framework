#ifndef GAME_H
#define GAME_H
#pragma once

#include "Engine.h"

class GameConfig
{
public:
	enum GRAPHICS_DEVICE
	{
		GRAPHICS_D3D,
		GRAPHICS_OPENGL
	};

	GameConfig();
	void Initialize(GRAPHICS_DEVICE gfx, int logicHz, int fpsLimit);

	GRAPHICS_DEVICE m_gfx;
	int m_logicHz;
	int m_fpsLimit;
};

class Game
{
public:
	Game();
	virtual ~Game();

	virtual GameConfig CreateConfig();

	virtual void HandleInitialize(Engine* engine);
	virtual void HandleShutdown();
	virtual void HandleSceneLoadBegin();
	virtual void HandleSceneLoadEnd();
	virtual void HandleSceneUnLoadBegin();
	virtual void HandleSceneUnLoadEnd();
	virtual void HandleGameStart();
	virtual void HandleGameEnd();
	virtual void HandlePlayerConnect(Client* client);
	virtual void HandleUpdateServer(float dt);
	virtual void HandleUpdateClient(float dt);
	virtual void HandleRender();

	const GameConfig& GetConfig();

protected:
	Engine* GetEngine();

private:
	GameConfig m_cfg;
	Engine* m_engine;
};

#endif
