#include "Calc.h"
#include "Game.h"
#include "Log.h"
#include "Particle.h"
#include "PolledTimer.h"
#include "Thingy.h"
#include "Timer.h"
#include "Types.h"

#define MAX_THINGIES 20

Game* gGame = 0;

Game::Game()
{
	mParticleMgr = new ParticleMgr(0.0f, 100.0f, 200, HandleParticleSegment, this);
	mSpawnTimer = new PolledTimer();
	mSpawnTimer->Initialize(&g_TimerRT);
	mThingyList = new Thingy[MAX_THINGIES];
	mPlayerState[0] = new PlayerState();
	mPlayerState[1] = new PlayerState();
}

Game::~Game()
{
	delete mPlayerState[0];
	mPlayerState[0] = 0;
	delete mPlayerState[1];
	mPlayerState[1] = 0;
	
	delete[] mThingyList;
	mThingyList = 0;
	
	delete mSpawnTimer;
	mSpawnTimer = 0;
	
	delete mParticleMgr;
	mParticleMgr = 0;
}

void Game::Begin()
{
	mSpawnTimer->SetInterval(0.1f);
	mSpawnTimer->Start();
	
	Vec2F pos[7] =
	{
		Vec2F(105, 110),
		Vec2F(215, 110),
		Vec2F(40, 220),
		Vec2F(160, 220),
		Vec2F(280, 220),
		Vec2F(90, 330),
		Vec2F(230, 330)
	};
	
	int idx = 0;
	
	for (int i = 0; i < 7; ++i)
	{
		Vec2F position = pos[i];//(Calc::Random(0.0f, 320.0f), Calc::Random(0.0f, 480.0f));
		Vec2F normal = Vec2F::FromAngle(Calc::mPI2 + Calc::Random(-1.0f, +1.0f) / 1.5f);
		float distance = normal * position;
		
		mThingyList[idx].Setup(position[0], position[1], normal[0], normal[1], distance, 60.0f, true);
		
		idx++;
	}
	
	mThingyList[idx++].Setup(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 10000.0f, false);
	mThingyList[idx++].Setup(0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 10000.0f, false);
	mThingyList[idx++].Setup(320.0f, 0.0f, 1.0f, 0.0f, 320.0f, 10000.0f, false);
	mThingyList[idx++].Setup(0.0f, 480.0f, 0.0f, 1.0f, 480.0f, 10000.0f, false);
}

void Game::Update(float dt)
{
	while (mSpawnTimer->ReadTick())
	{
		Particle& p = mParticleMgr->Allocate();
		
		p.Setup(160.0f, 0.0f, 0.0f, 0.0f, 2.1f, 0.7f);
	}
	
	mParticleMgr->Update(dt);
}

void Game::Render()
{
	mParticleMgr->Render();
	
	for (int i = 0; i < MAX_THINGIES; ++i)
	{
		Thingy& t = mThingyList[i];
		
		if (!t.mIsActive)
			continue;
		
		t.Render();
	}
}

Thingy* Game::HitThingy(float x, float y, float r)
{
	for (int i = 0; i < MAX_THINGIES; ++i)
	{
		Thingy& t = mThingyList[i];
		
		if (!t.mIsActive)
			continue;
		if (!t.mIsInteractive)
			continue;
		
		float distance = t.DistanceTo(x, y);
		
		if (distance <= r)
			return &t;
	}
	
	return 0;
}

PlayerState* Game::GetplayerState(int idx)
{
	return mPlayerState[idx];
}

bool Game::HandleParticleSegment(void* obj, Particle* p, float x1, float y1, float x2, float y2)
{
	Game* self = (Game*)obj;
	
	bool result = true;
	
	for (int i = 0; i < MAX_THINGIES; ++i)
	{
		Thingy& t = self->mThingyList[i];
		
		if (!t.mIsActive)
			continue;
		
		float d1 = t.PlaneDistanceTo(x1, y1);
		float d2 = t.PlaneDistanceTo(x2, y2);
		
		if (Calc::Sign(d1) == Calc::Sign(d2))
			continue;
		
		if (t.DistanceTo(x1, y1) > t.mSize)
			continue;
		
//		LOG_DBG("particle crossed thingy plane", 0);
		
		p->Reflect(t.mPlaneX, t.mPlaneY);
		
		result = false;
	}
	
	if (y2 > 480.0f)
	{
		int idx = Calc::Mid((int)(x2 / 160.0f), 0, 1);
		
//		LOG_DBG("score: %d", idx);
		
		p->mIsAlive = false;
		
		self->mPlayerState[idx]->mScore++;
	}
	
	return result;
}
