#include "Calc.h"
#include "FileStream.h"
#include "framework.h"
#include "StreamReader.h"
#include "StringEx.h"
#include "Ease.h"
#include <time.h>

#define GFX_SX 1920
#define GFX_SY 1080

struct VocabEntry
{
	std::string lang1;
	std::string lang2;
};

struct VocabList
{
	std::vector<VocabEntry> entries;
	
	void load(const char * filename)
	{
		try
		{
			entries.clear();

			FileStream stream(filename, (OpenMode)(OpenMode_Read | OpenMode_Text));
			StreamReader reader(&stream, false);
			const std::vector<std::string> lines = reader.ReadAllLines();
			for (auto & line : lines)
			{
				const auto parts = String::Split(line, '\t');
				if (parts.size() != 2)
					continue;
				VocabEntry entry;
				entry.lang1 = parts[0];
				entry.lang2 = parts[1];
				entries.push_back(entry);
			}
		}
		catch (std::exception & e)
		{
		}
	}
};

static VocabList vocabList;

static void drawPic(Sprite & sprite, const float t, const float alpha, const float zoom1, const float zoom2)
{
	setColorf(1.f, 1.f, 1.f, alpha);
	const float scaleX = GFX_SX / (float)sprite.getWidth();
	const float scaleY = GFX_SY / (float)sprite.getHeight();
	const float scale = Calc::Max(scaleX, scaleY);
	const float sx = sprite.getWidth() * scale;
	const float sy = sprite.getHeight() * scale;
	const float px = GFX_SX - sx;
	const float py = GFX_SY - sy;
	const float pt = EvalEase(t, kEaseType_SineInOut, 0.f);
	const float zoom = Calc::Lerp(zoom1, zoom2, pt);
	gxPushMatrix();
	{
		gxTranslatef(px * pt, py * pt, 0.f);
		gxTranslatef(+sx/2.f, +sy/2.f, 0.f);
		gxScalef(zoom, zoom, 1.f);
		gxTranslatef(-sx/2.f, -sy/2.f, 0.f);

		sprite.drawEx(0.f, 0.f, 0.f, scale, scale, true, FILTER_LINEAR);
		setColor(colorWhite);
	}
	gxPopMatrix();
}

static void wordTransform()
{
	gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);
	gxRotatef(sin(framework.time) * 20.f, 0.f, 0.f, 1.f);
}

static void drawText(float offset, const char * format, ...)
{
	char text[1024];
	va_list args;
	va_start(args, format);
	vsprintf_s(text, sizeof(text), format, args);
	va_end(args);

	const int fontSize = 150;
	drawText(0.f, offset * fontSize, fontSize, 0, 0, "%s", text);
}

static void gameMatch3()
{
	const int words[3] =
	{
		rand() % vocabList.entries.size(),
		rand() % vocabList.entries.size(),
		rand() % vocabList.entries.size()
	};

	const int index = rand() % 3;

	bool done = false;
	int guess = -1;
	float time = 4.f;

	do
	{
		framework.process();

		if (guess == -1)
		{
			if (keyboard.wentDown(SDLK_1))
				guess = 0;
			if (keyboard.wentDown(SDLK_2))
				guess = 1;
			if (keyboard.wentDown(SDLK_3))
				guess = 2;
		}
		else
		{
			time -= framework.timeStep;

			if (time <= 0.f)
			{
				time = 0.f;
				done = true;
			}
		}

		framework.beginDraw(0, 0, 0, 0);
		{
			wordTransform();

			setFont("calibri.ttf");

			setColor(colorWhite);
			drawText(-2.f, "%s", vocabList.entries[words[index]].lang1.c_str());
			
			setColor(guess == 0 ? ((guess == index) ? colorYellow : colorRed) : colorWhite);
			drawText(-1.f, "1 %s", vocabList.entries[words[0]].lang2.c_str());
			
			setColor(guess == 1 ? ((guess == index) ? colorYellow : colorRed) : colorWhite);
			drawText( 0.f, "2 %s", vocabList.entries[words[1]].lang2.c_str());

			setColor(guess == 2 ? ((guess == index) ? colorYellow : colorRed) : colorWhite);
			drawText(+1.f, "3 %s", vocabList.entries[words[2]].lang2.c_str());
		}
		framework.endDraw();
	} while (!done);
}

int main(int argc, char * argv[])
{
	time_t t;
	srand(time(&t));

	Calc::Initialize();

	changeDirectory("data");

	framework.fullscreen = true;
	framework.exclusiveFullscreen = false;
	framework.useClosestDisplayMode = true;

	mouse.showCursor(false);

	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		vocabList.load("ro-en-1000.txt");
		
		const auto pics = listFiles("_pics", true);

		int currentEntry = -1;
		int progress = 0;
		float timer = 0.f;

		struct PicInfo
		{
			PicInfo()
				: index(-1)
				, zoom1(0.f)
				, zoom2(0.f)
			{
			}

			int index;
			float zoom1;
			float zoom2;
		} picInfo[2];
		float picTimer = 0.f;
		float picTimerRcp = 0.f;

		while (!framework.quitRequested)
		{
			//gameMatch3();

			framework.process();
			
			if (keyboard.wentDown(SDLK_ESCAPE))
				framework.quitRequested = true;

			timer -= framework.timeStep;
			if (timer < 0.f)
				timer = 0.f;

			picTimer -= framework.timeStep;
			if (picTimer < 0.f)
				picTimer = 0.f;

			if (picTimer == 0.f || mouse.wentDown(BUTTON_RIGHT))
			{
				if (!pics.empty())
				{
					picInfo[0] = picInfo[1];
					do
					{
						picInfo[1].index = rand() % pics.size();
						picInfo[1].zoom1 = Calc::Random(1.f, 1.5f);
						picInfo[1].zoom2 = Calc::Random(1.f, 1.5f);
						picTimer = 20.f;
						picTimerRcp = 1.f / picTimer;
					}
					while (picInfo[1].index == picInfo[0].index && pics.size() != 1);
				}
			}

			if (mouse.wentDown(BUTTON_LEFT) || timer == 0.f)
			{
				if (!vocabList.entries.empty())
				{
					currentEntry = rand() % vocabList.entries.size();
					progress = 0;
					//timer = cos(framework.time / 120.f * Calc::m2PI) * 2.f + 3.f;
					timer = 4.f;
				}
			}

			framework.beginDraw(0, 0, 0, 0);
			{
				setBlend(BLEND_ALPHA);

				if (picInfo[0].index != -1)
				{
					Sprite oldPic(pics[picInfo[0].index].c_str());
					drawPic(oldPic, 1.f, 1.f, picInfo[0].zoom1, picInfo[0].zoom2);
				}
				if (picInfo[1].index != -1)
				{
					Sprite newPic(pics[picInfo[1].index].c_str());
					drawPic(newPic, 1.f - picTimer * picTimerRcp, Calc::Mid((1.f / picTimerRcp) - picTimer, 0.f, 1.f), picInfo[1].zoom1, picInfo[1].zoom2);
				}

				setFont("calibri.ttf");
				setColor(colorWhite);
				
				if (currentEntry != -1)
				{
					gxPushMatrix();
					{
						gxTranslatef(GFX_SX/2, GFX_SY/2, 0.f);
						gxRotatef(sin(framework.time) * 20.f, 0.f, 0.f, 1.f);

						for (int i = -1; i <= +1; ++i)
						{
							const int index = (currentEntry + i + vocabList.entries.size()) % vocabList.entries.size();

							const int fontSize = 150;
							drawText(0.f, - fontSize * 4/9 + fontSize * 4/2 * i, fontSize, 0, 0, "%s", vocabList.entries[index].lang1.c_str());
							drawText(0.f, + fontSize * 4/9 + fontSize * 4/2 * i, fontSize, 0, 0, "%s", vocabList.entries[index].lang2.c_str());
						}
					}
					gxPopMatrix();
				}
			}
			framework.endDraw();
		}
		
		framework.shutdown();
	}
	
	return 0;
}
