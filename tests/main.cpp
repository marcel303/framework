#include "framework.h"
#include "mediaplayer/MPUtil.h"
#include "ui.h"

extern const int GFX_SX;
extern const int GFX_SY;

#define FULLSCREEN 0

#if FULLSCREEN
const int GFX_SX = 1920;
const int GFX_SY = 1080;
#else
const int GFX_SX = 1024;
const int GFX_SY = 768;
#endif

extern void testMenu();

int main(int argc, char * argv[])
{
	setupPaths(CHIBI_RESOURCE_PATHS);
	
	framework.enableRealTimeEditing = true;
	
	framework.enableDepthBuffer = true;
	framework.enableDrawTiming = false;
	//framework.enableProfiling = true;
	
#if FULLSCREEN
	framework.fullscreen = true;
	//framework.exclusiveFullscreen = true;
#endif
	
	if (framework.init(GFX_SX, GFX_SY))
	{
	#if !defined(DEBUG)
		framework.fillCaches(true);
	#endif
		
		initUi();
		
		MP::Util::InitializeLibAvcodec();

		testMenu();
	}

	Font("calibri.ttf").saveCache();
	
	shutUi();
	
	framework.shutdown();

	return 0;
}
