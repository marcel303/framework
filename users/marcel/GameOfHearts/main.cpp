#include "Calc.h"
#include "framework.h"

#if !defined(DEBUG)
	#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

const int gsx = 640;
const int gsy = 480;
const float playerScale = 1.66f / 10.f;
const float playerScaleSel = 2.f / 10.f;
const float playerSpeed = 140.f;
const float initLoveTime = .5f;
const float accel = 1.f / 3.f;
const Color loveColor = Color::fromHex("ff56c1");
const float spawnTime = 3.f;
const float heartSpeed = 70.f;
const float randTime = 5.f;
const int maxHearts = 18;

static float loveTime = 0.f;
static float loveTimer = 0.f;
static float spawnTimer = 0.f;
static int numHearts = 0;

struct Game
{
	static const int kMaxHearts = 32;

	enum State
	{
		kState_PlayerSelect,
		kState_Play,
		kState_Ending
	};

	enum Gender
	{
		kGender_Female,
		kGender_Male
	};

	struct Heart
	{
		Heart()
			: active(false)
		{
		}

		bool active;
		float x;
		float y;
		float v;
		float vx;
		float vy;
		float randtime;
		float initRandTime;
	};

	struct Player
	{
		float x;
		float y;

		bool hittest(float x, float y) const
		{
			return
				x >= this->x - 180.f * playerScale &&
				y >= this->y - 830.f * playerScale &&
				x <= this->x + 180.f * playerScale &&
				y <= this->y;
		}

		void handleScore()
		{
			Sound("heart.ogg").play();

			loveTime = initLoveTime * (1.f + numHearts * accel);
			loveTimer = loveTime;
			numHearts++;
		}
	};

	State m_state;
	float m_stateTime;
	Gender m_gender;
	Heart m_hearts[kMaxHearts];
	Player m_player;

	Game()
		: m_state(kState_PlayerSelect)
		, m_stateTime(0.f)
		, m_gender(kGender_Female)
	{
	}

	void spawnHeart()
	{
		for (;;)
		{
			const float x = rand() % gsx;
			const float y = rand() % gsy;
			const float a = (rand() % 1024) / 1023.f * 2.f * M_PI;
			const float v = heartSpeed * (1.f + numHearts * accel);
			const float vx = sinf(a) * v;
			const float vy = cosf(a) * v;

			if (!m_player.hittest(x, y))
			{
				for (int i = 0; i < kMaxHearts; ++i)
				{
					if (!m_hearts[i].active)
					{
						m_hearts[i].active = true;
						m_hearts[i].x = x;
						m_hearts[i].y = y;
						m_hearts[i].v = v;
						m_hearts[i].vx = vx;
						m_hearts[i].vy = vy;
						m_hearts[i].randtime = 0.f;
						if (numHearts < 8)
							m_hearts[i].initRandTime = 0.f;
						else
							m_hearts[i].initRandTime = randTime / (1.f + (numHearts - 8) * accel);
						m_hearts[i].randtime = m_hearts[i].initRandTime;
						break;
					}
				}

				return;
			}
		}
	}

	void tick(float dt)
	{
		m_stateTime += dt;

		State oldState = m_state;

		switch (m_state)
		{
		case kState_PlayerSelect:
			if (keyboard.wentDown(SDLK_LEFT))
			{
				Sound("select.ogg").play();
				m_gender = (Gender)((m_gender + 2 - 1) % 2);
			}
			if (keyboard.wentDown(SDLK_RIGHT))
			{
				Sound("select.ogg").play();
				m_gender = (Gender)((m_gender + 2 + 1) % 2);
			}
			if (keyboard.wentDown(SDLK_SPACE) || keyboard.wentDown(SDLK_RETURN))
			{
				m_state = kState_Play;
				m_stateTime = 0.f;
			}
			break;

		case kState_Play:
			loveTimer = Calc::Max(0.f, loveTimer - dt);
			spawnTimer = Calc::Min(spawnTime, spawnTimer + dt);

			//if (m_stateTime > 2.f)
			{
				if (keyboard.isDown(SDLK_LEFT))
					m_player.x -= playerSpeed * dt;
				if (keyboard.isDown(SDLK_RIGHT))
					m_player.x += playerSpeed * dt;
				if (keyboard.isDown(SDLK_UP))
					m_player.y -= playerSpeed * dt;
				if (keyboard.isDown(SDLK_DOWN))
					m_player.y += playerSpeed * dt;

				if (m_player.x < 0.f)
					m_player.x = gsx;
				if (m_player.y < 0.f)
					m_player.y = gsy;
				if (m_player.x > gsx)
					m_player.x = 0;
				if (m_player.y > gsy)
					m_player.y = 0;
			}

		#if 0
			if (spawnTimer == spawnTime)
			{
				spawnTimer = 0.f;

				spawnHeart();
			}
		#else
			{
				bool spawn = true;
				spawn &= numHearts < maxHearts;
				for (int i = 0; i < kMaxHearts; ++i)
					spawn &= !m_hearts[i].active;
				if (spawn)
					spawnHeart();
			}
		#endif

			for (int i = 0; i < kMaxHearts; ++i)
			{
				if (m_hearts[i].active)
				{
					if (m_player.hittest(m_hearts[i].x, m_hearts[i].y))
					{
						m_hearts[i].active = false;

						m_player.handleScore();

						if (numHearts == maxHearts)
						{
							Sound("recorder.ogg").play();

							m_stateTime = 0.f;
						}
					}
					else
					{
						m_hearts[i].x += m_hearts[i].vx * dt;
						m_hearts[i].y += m_hearts[i].vy * dt;

						if (m_hearts[i].x < 0.f)
							m_hearts[i].x = gsx;
						if (m_hearts[i].y < 0.f)
							m_hearts[i].y = gsy;
						if (m_hearts[i].x > gsx)
							m_hearts[i].x = 0;
						if (m_hearts[i].y > gsy)
							m_hearts[i].y = 0;

						m_hearts[i].randtime = Calc::Max(0.f, m_hearts[i].randtime - dt);
						if (m_hearts[i].randtime == 0.f && m_hearts[i].initRandTime != 0.f)
						{
							const float a = (rand() % 1024) / 1023.f * 2.f * M_PI;
							const float v = heartSpeed * (1.f + numHearts * accel);
							m_hearts[i].vx = sinf(a) * v;
							m_hearts[i].vy = cosf(a) * v;
							m_hearts[i].randtime = m_hearts[i].initRandTime;
							if (m_hearts[i].initRandTime > 1.f / 60.f)
								m_hearts[i].initRandTime /= 2.f;
							m_hearts[i].v *= 3.f / 4.f;
						}
					}
				}
			}

			if (numHearts == maxHearts)
			{
				if (m_stateTime >= 5.f)
				{
					m_state = kState_Ending;
					m_stateTime = 0.f;
				}
			}
			break;

		case kState_Ending:
			break;
		}

		if (m_state != oldState)
		{
			if (m_state == kState_Play)
			{
				m_player.x = gsx * 1 / 3;
				m_player.y = gsy - 100;
			}

			loveTimer = 0.f;
			spawnTimer = 0.f;
			numHearts = 0;
		}
	}

	void draw()
	{
		switch (m_state)
		{
		case kState_PlayerSelect:
			gxPushMatrix();
			gxTranslatef(0.f, 70.f, 0.f);
			Sprite("female.png").drawEx(gsx * 1/3, gsy/2, 0.f, playerScaleSel, playerScaleSel);
			Sprite("male.png").drawEx(gsx * 2/3, gsy/2, 0.f, playerScaleSel, playerScaleSel);
			if (m_gender == kGender_Female)
				drawText(gsx * 1/3, gsy/2, 24, 0, 1, "Bride");
			if (m_gender == kGender_Male)
				drawText(gsx * 2/3, gsy/2, 24, 0, 1, "Groom");
			gxPopMatrix();
			break;

		case kState_Play:
			if (numHearts == maxHearts)
			{
				if (fmodf(m_stateTime, .3f) <= .15f)
					Sprite("female.png").drawEx(m_player.x, m_player.y, 0.f, playerScale, playerScale);
				else
					Sprite("male.png").drawEx(m_player.x, m_player.y, 0.f, playerScale, playerScale);
			}
			else
			{
				if (m_gender == kGender_Female)
					Sprite("female.png").drawEx(m_player.x, m_player.y, 0.f, playerScale, playerScale);
				if (m_gender == kGender_Male)
					Sprite("male.png").drawEx(m_player.x, m_player.y, 0.f, playerScale, playerScale);
			}

			for (int i = 0; i < kMaxHearts; ++i)
			{
				if (m_hearts[i].active)
				{
					Sprite("heart.png").drawEx(m_hearts[i].x, m_hearts[i].y);
				}
			}

			setColor(loveColor);
			drawRect(gsx/2 - 100, 50, gsx/2 - 100 + 200 * numHearts / float(maxHearts), 100);
			drawRectLine(gsx/2 - 100, 50, gsx/2 + 100, 100);
			setColor(colorWhite);
			drawText(gsx/2, 75, 20, 0, 0, "%d / %d HEARTS", numHearts, maxHearts);

		#if 0
			drawRectLine(
				m_player.x - 180.f * playerScale,
				m_player.y - 830.f * playerScale,
				m_player.x + 180.f * playerScale,
				m_player.y);
		#endif
			break;

		case kState_Ending:
			gxPushMatrix();
			gxTranslatef(0.f, -64.f, 0.f);
			drawText(gsx/2, gsy/2, 20, 0, 0, "After collecting so many hearts..");
			drawText(gsx/2, gsy/2 + 32, 20, 0, 0, "..they were ready..");
			drawText(gsx/2, gsy/2 + 64, 32, 0, 0, "And they got married!");
			drawText(gsx/2, gsy/2 + 96, 20, 0, 0, "(the end)");
			gxPopMatrix();
			break;
		}
	}
};

int main(int argc, char * argv[])
{
	framework.fullscreen = false;

	if (!framework.init(0, 0, gsx, gsy))
	{
		showErrorMessage("Startup Error", "Failed to initialize framework.");
	}
	else
	{
		changeDirectory("data");

		framework.fillCachesWithPath(".", false);

		Game game;

		bool stop = false;

		while (!stop)
		{
			framework.process();

			if (keyboard.wentDown(SDLK_ESCAPE))
				stop = true;

			game.tick(framework.timeStep);

			const Color backColor = colorBlack.addRGB(loveColor.mulRGB(loveTimer / loveTime));

			framework.beginDraw(backColor.r * 255.f, backColor.g * 255.f, backColor.b * 255.f, 0);
			{
				setFont("calibri.ttf");

				game.draw();
			}
			framework.endDraw();
		}

		framework.shutdown();
	}

	return 0;
}
