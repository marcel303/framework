#include <math.h>
#include <time.h>
#include "enemy.h"
#include "eventmgr.h"
#include "game.h"
#include "input.h"
#include "main.h"
#include "powerup.h"
#include "render_al.h"
#include "render_gl.h"
#ifdef WIN32
#include <SDL/SDL.h> // main
#include "render_gl.h"
#endif
#ifdef IPHONEOS
#include "render_iphone.h"
#endif
#include "sounds.h"
#include "world.h"

#define UPDATE_INTERVAL (1.0f / 60.0f)

static PowerupType GetPowerupType();

void Main::Initialize()
{
	srand((unsigned int)time(0));

	SoundInit();

	gEventMgr = new EventMgr();
	
#ifdef WIN32
	//gRender = new RenderAL();
	gRender = new RenderGL();
#endif
#ifdef IPHONEOS
	gRender = new RenderIphone();
#endif
	//gRender->Init(640, 480);
	//gRender->Init(480, 320);
#ifdef WIN32
	gRender->Init(480, 320);
#endif
#ifdef IPHONEOS
	gRender->Init(320, 480);
#endif

	gGame = new Game();
	gWorld = new World();

	//for (int i = 0; i < 100; ++i)
	for (int i = 0; i < 0; ++i)
	{
		{
			Powerup* powerup = gWorld->AllocatePowerup();
			powerup->Make(PowerupType_Red);
		}
		{
			Powerup* powerup = gWorld->AllocatePowerup();
			powerup->Make(PowerupType_Missiles);
		}
		{
			Powerup* powerup = gWorld->AllocatePowerup();
			powerup->Make(PowerupType_Orange);
		}
		{
			Powerup* powerup = gWorld->AllocatePowerup();
			powerup->Make(PowerupType_Shockwave);
		}
	}
}

void Main::Loop()
{
	gGame->Update(UPDATE_INTERVAL);
	
	gWorld->Update(UPDATE_INTERVAL);

	while (gWorld->mPowerupList.size() < 7)
	{
		Powerup* powerup = gWorld->AllocatePowerup();
		powerup->Make(GetPowerupType());
	}

	if (gGame->IsPlaying_get())
	{
		static float chargeTime = 0.0f;

		//size_t enemyCount = mTime * 20.0f / 10.0f;
		size_t maxEnemyCount = (size_t)(gGame->Time_get() * 10.0f / 10.0f);
		const float chargeTime2 = 5.0f / maxEnemyCount;

		while (gWorld->mEnemyList.size() < maxEnemyCount && chargeTime >= 0.0f)
		{
			Enemy* enemy = gWorld->AllocateEnemy();
			enemy->Make_Spinner();

			chargeTime -= chargeTime2;
		}

		if (gWorld->mEnemyList.size() != maxEnemyCount)
			chargeTime += UPDATE_INTERVAL;
	}

//	gRender->Clear();
	gRender->Fade(5);

	gWorld->Render();

	gRender->Text(0.0f, 0.0f, Color(1.0f, 1.0f, 1.0f), "SCORE: %06d", gGame->Score_get());

	if (!gGame->IsPlaying_get())
	{
		gRender->Text(40.0f, 240.0f, Color(1.0f, 1.0f, 1.0f), "GAME OVER");
	}

	gRender->Present();
}
	
void Main::Shutdown()
{
	delete gWorld;
	gWorld = 0;
	delete gGame;
	gGame = 0;
	delete gRender;
	gRender = 0;
	
	delete gEventMgr;
	gEventMgr = 0;
	
	SoundShutdown();
}

#ifdef WIN32

int main(int argc, char* argv[])
{
	Main main;

	main.Initialize();

	while (!gKeyboard.Get(KeyCode_Escape))
	{
		main.Loop();
	}

	main.Shutdown();

	return 0;
}

#endif

static PowerupType GetPowerupType()
{
	//return PowerupType_Missiles;
	return (PowerupType)(rand() % PowerupType__Count);
}

/*float Random(float min, float max)
{
	return min + (max - min) * (rand() & 4095) / 4095.0f;
}*/
