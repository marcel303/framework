#include "eventmgr.h"
#include "game.h"
#include "player.h"
#include "sounds.h"
#include "world.h"

Game* gGame = 0;

Game::Game()
{
	mScore = 0;
	mIsPlaying = false;
	mTime = 0.0f;
}

void Game::Begin()
{
	mScore = 0;
	mIsPlaying = true;
	mTime = 0.0f;
	
	gWorld->Begin();

	SoundPlay(Sound_Begin1);
	
	gEventMgr->Push(Event(EventType_Begin));
}

void Game::End()
{
	gWorld->mPlayer->CommitCombo();
	
	mIsPlaying = false;

	SoundPlay(Sound_End1);
	
	gEventMgr->Push(Event(EventType_End));
}

void Game::Update(float dt)
{
	mTime += dt;
}

int Game::Score_get() const
{
	return mScore;
}

void Game::Score_add(int amount)
{
	if (!mIsPlaying)
		return;

	mScore += amount;
}

bool Game::IsPlaying_get() const
{
	return mIsPlaying;
}

float Game::Time_get() const
{
	return mTime;
}
