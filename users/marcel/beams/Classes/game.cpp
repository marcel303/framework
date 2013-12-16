#include "board.h"
#include "game.h"
#include "paths.h"
#include "texture.h"

Game* gGame = 0;

Game::Game()
{
	mBoard = new Board();
}

Game::~Game()
{
	delete mBoard;
	mBoard = 0;
}

void Game::Attach(CallBack onAnimationTrigger)
{
	mOnAnimationTrigger = onAnimationTrigger;
}

void Game::Begin()
{
	// todo: load texture
	
	//TextureRGBA* texture = CreateTexture(512);
	TextureRGBA* back = LoadTexture_TGA("back.tga");
	TextureRGBA* picture = LoadTexture_TGA("picture1.tga");
	
	// todo: setup board
	
	mBoard->Setup(CallBack(this, HandleAnimationTrigger), 6, 6, back, picture, true);
	
	delete back;
	delete picture;
	
	// todo: load level description
}

void Game::End()
{
}

Board* Game::Board_get()
{
	return mBoard;
}

void Game::HandleAnimationTrigger(void* obj, void* arg)
{
	Game* self = (Game*)obj;
	
	if (self->mOnAnimationTrigger.IsSet())
		self->mOnAnimationTrigger.Invoke(0);
}
