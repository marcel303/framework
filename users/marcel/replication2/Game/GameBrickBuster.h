#ifndef GAMEBRICKBUSTER_H
#define GAMEBRICKBUSTER_H
#pragma once

#include "Game.h"

class GameBrickBuster : public Game
{
public:
	GameBrickBuster();

	virtual GameConfig CreateConfig();

	virtual void HandleSceneLoadBegin();
	virtual void HandlePlayerConnect(Client* client);
};

#endif
