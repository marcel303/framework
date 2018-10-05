#include "framework.h"
#include "objects/audioSourceVorbis.h"
#include "objects/binauralizer.h"
#include "objects/binaural_cipic.h"
#include "objects/paobject.h"
#include "Quat.h"
#include "soundmix.h"
#include "soundVolume.h"
#include "vfxGraph.h"
#include "vfxNodes/vfxNodeDisplay.h"
#include "video.h"
#include <atomic>

extern const int GFX_SX;
extern const int GFX_SY;

#if !defined(DEBUG)
const int GFX_SX = 1280*2;
const int GFX_SY = 800*2;
#elif 0
const int GFX_SX = 2400;
const int GFX_SY = 1200;
#else
const int GFX_SX = 640;
const int GFX_SY = 480;
#endif

#include "860-thetribe-cagedsounds.cpp"
#include "860-thetribe-spokenword.cpp"
#include "860-thetribe-video360.cpp"
#include "860-thetribe-videomixer.cpp"
#include "860-thetribe-videotube.cpp"

int main(int argc, char * argv[])
{
	changeDirectory(SDL_GetBasePath());
	
#if !defined(DEBUG)
	framework.fullscreen = true;
	framework.exclusiveFullscreen = false;
#endif

    framework.enableDepthBuffer = true;
    framework.enableRealTimeEditing = true;
    
	if (!framework.init(0, nullptr, GFX_SX, GFX_SY))
		return -1;
    
    //Cagedsounds::main();
    //Spokenword::main();
    //Video360::main();
    //Videomixer::main();
    Videotube::main();
	
	Font("calibri.ttf").saveCache();
	
	framework.shutdown();
	
	return 0;
}
