#include "framework.h"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 400;
const int GFX_SY = 400;

extern void testImu9250();

int main(int argc, char * argv[])
{
    const char * basePath = SDL_GetBasePath();
    changeDirectory(basePath);
    
	framework.enableDepthBuffer = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		testImu9250();
	}

	framework.shutdown();

	return 0;
}
