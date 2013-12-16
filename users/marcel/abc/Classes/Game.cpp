#include "Game.h"
#include "HackRender.h"

Game* gGame = 0;

Game::Game()
{
	mTime = 0.0f;
}

void Game::Setup()
{
	mViewer.Setup(Vec2F(MAP_SX * TILE_SX, MAP_SY * TILE_SY), Vec2F(320.0f, 480.0f));
	mMap.Setup();
	mPlayer.Setup();
}

void Game::Update(float dt)
{
	mMap.Update(dt);
	mPlayer.Update(dt);
	mViewer.SetFocus(mPlayer.mPosition);
	
	mTime += dt;
}

void Game::Render()
{
	mViewer.Apply();
	mMap.Render();
	mPlayer.Render();
	
	HR_SetTranslation(Vec2F(0.0f, 0.0f));
	
	mPlayer.RenderUI();
}

float Game::Time_get() const
{
	return mTime;
}