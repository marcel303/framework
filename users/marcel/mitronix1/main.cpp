#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "framework.h"
#include "slideshow.h"
#include "soundmix.h"

extern const int GFX_SX;
extern const int GFX_SY;
const int GFX_SX = 800;
const int GFX_SY = 300;

struct Instrument
{
	AudioGraphInstance * audioGraphInstance;
	
	Instrument(const char * filename)
		: audioGraphInstance(nullptr)
	{
		audioGraphInstance = g_audioGraphMgr->createInstance(filename);
	}
	
	~Instrument()
	{
		g_audioGraphMgr->free(audioGraphInstance);
	}
};

int main(int argc, char * argv[])
{
	if (framework.init(0, 0, GFX_SX, GFX_SY))
	{
		SDL_mutex * audioMutex = SDL_CreateMutex();
		
		AudioVoiceManager * audioVoiceMgr = new AudioVoiceManager();
		audioVoiceMgr->init(16, 16);
		audioVoiceMgr->outputStereo = true;
		g_voiceMgr = audioVoiceMgr;
		
		AudioGraphManager * audioGraphMgr = new AudioGraphManager();
		audioGraphMgr->init(audioMutex);
		g_audioGraphMgr = audioGraphMgr;
		
		AudioUpdateHandler * audioUpdateHandler = new AudioUpdateHandler();
		audioUpdateHandler->init(audioMutex, nullptr, 0);
		
		PortAudioObject * paObject = new PortAudioObject();
		if (paObject->init(SAMPLE_RATE, 2, 2, AUDIO_UPDATE_SIZE, audioUpdateHandler) == false)
			logError("failed to initialize port audio object");
		
		Slideshow slideshow;
		slideshow.pics = listFiles("source-images", false);
		
		Instrument * instrument = new Instrument("mtx1.xml");
		
		audioGraphMgr->selectInstance(instrument->audioGraphInstance);
		//audioGraphMgr->selectInstance(nullptr);
		
		do
		{
			framework.process();
			
			const float dt = std::min(1.f / 15.f, framework.timeStep);
			
			slideshow.tick(dt);

			framework.beginDraw(0, 0, 0, 0);
			{
				setColor(colorWhite);
				slideshow.draw();
				
				setColor(colorWhite);
				pushFontMode(FONT_SDF);
				{
					setFont("calibri.ttf");
					drawText(GFX_SX/2, GFX_SY/2, 48, 0, 0, "Hello World");
				}
				popFontMode();
				
				audioGraphMgr->tickEditor(dt, false);
				audioGraphMgr->drawEditor();
			}
			framework.endDraw();
		} while (!keyboard.wentDown(SDLK_ESCAPE));
		
		delete instrument;
		instrument = nullptr;
		
		paObject->shut();
		delete paObject;
		paObject = nullptr;
		
		audioUpdateHandler->shut();
		delete audioUpdateHandler;
		audioUpdateHandler = nullptr;
		
		g_audioGraphMgr = nullptr;
		audioGraphMgr->shut();
		delete audioGraphMgr;
		audioGraphMgr = nullptr;
		
		audioVoiceMgr->shut();
		delete audioVoiceMgr;
		audioVoiceMgr = nullptr;
		
		SDL_DestroyMutex(audioMutex);
		audioMutex = nullptr;
		
		Font("calibri.ttf").saveCache();
		
		framework.shutdown();
	}
	
	return 0;
}
