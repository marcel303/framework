#pragma once

#include "td_forward.h"
#include "tower.h"

class Game
{
public:
	Game();
	~Game();
	
	void Begin(std::string levelPath);
	void End();
	
	void Update(float dt);
	void Render();
	
	Level* Level_get();
	float Time_get() const;
	TowerType TowerPlacementType_get() const;
	void TowerPlacementType_set(TowerType type);
	Tower* SelectedTower_get();
	void SelectedTower_set(Tower* tower);
	void DeselectTower(Tower* tower);
	void DeselectTower();
	
private:
	Level* mLevel;
	float mTime;
	TowerType mTowerPlacementType;
	Tower* mSelectedTower;
};

extern Game* gGame;
