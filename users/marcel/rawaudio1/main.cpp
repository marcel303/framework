#include "framework.h"

#define GFX_SX 1024
#define GFX_SY 600

#define NUM_SAMPLES 1000

static uint32_t s_samples[NUM_SAMPLES] = { };

static void readAudio(FILE * file, size_t position)
{
	fseek(file, position, SEEK_SET);
	
	for (int i = 0; i < NUM_SAMPLES; ++i)
	{
		uint8_t bytes[4];
		
		if (fread(bytes, 1, 3, file) == 3)
		{
			int32_t value =
				(bytes[0] << 8) |
				(bytes[1] << 16) |
				(bytes[2] << 24);
			
			value >>= 8;
			
			s_samples[i] = value;
		}
		else
		{
			s_samples[i] = 0;
		}
	}
	
}

int main(int argc, char * argv[])
{
	const char * filename = "/Users/thecat/Desktop/H4N_SD.dmg";
	
	FILE * file = fopen(filename, "rb");
	
	if (file == nullptr)
	{
		logError("failed to open %s", filename);
		return -1;
	}
	
	fseek(file, 0, SEEK_END);
	const size_t fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);
	
	size_t position = 0;
	int offset = 0;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		while (!framework.quitRequested)
		{
			framework.process();
			
			bool reload = false;
			
			if (mouse.isDown(BUTTON_LEFT))
			{
				position = uint64_t(mouse.x) * fileSize / GFX_SX;
				reload = true;
			}
			
			for (int i = 0; i < 6; ++i)
			{
				if (keyboard.wentDown((SDLKey)(SDLK_1 + i)))
				{
					offset = i;
					reload = true;
				}
			}
			
			if (reload)
			{
				readAudio(file, (position / 6 * 6) + offset);
			}
			
			framework.beginDraw(0, 0, 0, 0);
			{
				gxPushMatrix();
				gxTranslatef(0, GFX_SY/2, 0);
				gxScalef(GFX_SX / float(NUM_SAMPLES), GFX_SY/2, 1);
				
				setColor(colorWhite);
				gxBegin(GL_LINES);
				{
					for (int i = 0; i < NUM_SAMPLES - 1; i += 2)
					{
						const int index1 = i;
						const int index2 = i + 2;
						
						const int value1 = s_samples[index1];
						const int value2 = s_samples[index2];
						
						const float h1 = value1 / float(1 << 23);
						const float h2 = value2 / float(1 << 23);
						
						gxVertex2f(index1, h1);
						gxVertex2f(index2, h2);
					}
				}
				gxEnd();
				
				gxPopMatrix();
			}
			framework.endDraw();
		}
		
		framework.shutdown();
	}
	
	return 0;
}
