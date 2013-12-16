#include "FileStream.h"
#include "game.h"
#include "level.h"
#include "tower.h"

Game* gGame = 0;

#ifndef DEPLOYMENT
Tower* MakeTower(TowerType type, float x, float y)
{
	TowerDesc desc;
	desc.type = type;
	desc.position[0] = x;
	desc.position[1] = y;
	desc.level = 0;
	Tower* tower = new Tower();
	tower->Make(desc);
	return tower;
}
#endif

Game::Game()
{
	mLevel = 0;
	mTime = 0.0f;
	mTowerPlacementType = TowerType_Undefined;
	mSelectedTower = 0;
}

Game::~Game()
{
	delete mLevel;
	mLevel = 0;
}

void Game::Begin(std::string levelPath)
{
	Assert(!mLevel);
	
	mLevel = new Level(this);
	
	std::string pathPath = levelPath + "_path.txt";
	std::string wavePath = levelPath + "_waves.txt";
	
	FileStream pathStream;
	FileStream waveStream;
	
	pathStream.Open(pathPath.c_str(), OpenMode_Read);
	waveStream.Open(wavePath.c_str(), OpenMode_Read);

	mLevel->Load(&pathStream, &waveStream);
	
	mTime = 0.0f;
	
#ifndef DEPLOYMENT
	mLevel->AddTower(MakeTower(TowerType_Vulcan, 40.0f, 40.0f));
#endif
}

void Game::End()
{
	Assert(mLevel);
	
	delete mLevel;
	mLevel = 0;
}

void Game::Update(float dt)
{
	mLevel->Update(dt);
	
	mTime += dt;
}
 
void Game::Render()
{
	mLevel->Render();
}

Level* Game::Level_get()
{
	return mLevel;
}

float Game::Time_get() const
{
	return mTime;
}

TowerType Game::TowerPlacementType_get() const
{
	return mTowerPlacementType;
}

void Game::TowerPlacementType_set(TowerType type)
{
	mTowerPlacementType = type;
}

Tower* Game::SelectedTower_get()
{
	return mSelectedTower;
}

void Game::SelectedTower_set(Tower* tower)
{
	mSelectedTower = tower;
}

void Game::DeselectTower(Tower* tower)
{
	if (tower != mSelectedTower)
		return;
	
	DeselectTower();
}

void Game::DeselectTower()
{
	mSelectedTower = 0;
}
