#pragma once

#include "forward.h"

extern Game* gGame;

class Game
{
public:
	Game();

	void Begin();
	void End();
	void Update(float dt);

	int Score_get() const;
	void Score_add(int amount);

	bool IsPlaying_get() const;

	float Time_get() const;
	
private:
	int mScore;
	bool mIsPlaying;
	float mTime;
};
