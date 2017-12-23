#include "framework.h"
#include "mediaplayer/MPUtil.h"
#include "../libparticle/ui.h"

extern const int GFX_SX;
extern const int GFX_SY;

const int GFX_SX = 1024;
const int GFX_SY = 768;

extern void testMenu();

int main(int argc, char * argv[])
{
	framework.enableRealTimeEditing = true;
	
	framework.enableDepthBuffer = true;
	framework.enableDrawTiming = false;
	//framework.enableProfiling = true;
	
	if (framework.init(0, nullptr, GFX_SX, GFX_SY))
	{
		initUi();
		
		MP::Util::InitializeLibAvcodec();

		testMenu();
	}

	Font("calibri.ttf").saveCache();
	
	shutUi();
	
	framework.shutdown();

	return 0;
}
