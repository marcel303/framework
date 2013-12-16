#pragma once

#include <string>
class PlayerState
{
public:
	PlayerState()
	{
		mName = "Player";
		mScore = 0;
	}
	
	std::string mName;
	int mScore;
};

class Game
{
public:
	Game();
	~Game();
	
	void Begin();
	
	void Update(float dt);
	void Render();
	
	class Thingy* HitThingy(float x, float y, float r);
	class PlayerState* GetplayerState(int idx);
	
private:
	static bool HandleParticleSegment(void* obj, class Particle* p, float x1, float y1, float x2, float y2);
	
	class ParticleMgr* mParticleMgr;
	class PolledTimer* mSpawnTimer;
	class Thingy* mThingyList;
	class PlayerState* mPlayerState[2];
};

extern Game* gGame;
