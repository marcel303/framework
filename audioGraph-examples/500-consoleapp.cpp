/*
	Copyright (C) 2020 Marcel Smit
	marcel303@gmail.com
	https://www.facebook.com/marcel.smit981

	Permission is hereby granted, free of charge, to any person
	obtaining a copy of this software and associated documentation
	files (the "Software"), to deal in the Software without
	restriction, including without limitation the rights to use,
	copy, modify, merge, publish, distribute, sublicense, and/or
	sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following
	conditions:

	The above copyright notice and this permission notice shall be
	included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
	EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
	OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
	NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
	HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
	WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
	OTHER DEALINGS IN THE SOFTWARE.
*/

#include "audioGraph.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "Debugging.h"
#include <thread>

#if defined(MACOS) || defined(LINUX)
	#include <unistd.h>
#endif

#if defined(WINDOWS)
	#include <direct.h>
#endif

#ifdef WIN32
	#define chdir _chdir
#endif

#define CHANNEL_COUNT 64

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	if (chdir(CHIBI_RESOURCE_PATH) != 0)
		return -1;
#endif

	// initialize audio related systems
	
	AudioMutex mutex;
	mutex.init();

	AudioVoiceManagerBasic voiceMgr;
	voiceMgr.init(&mutex, CHANNEL_COUNT);
	voiceMgr.outputStereo = true;

	AudioGraphManager_Basic audioGraphMgr(true);
	audioGraphMgr.init(&mutex, &voiceMgr);

	AudioUpdateHandler audioUpdateHandler;
	audioUpdateHandler.init(&mutex, &voiceMgr, &audioGraphMgr);

	PortAudioObject pa;
	pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler);
	
	// create an audio graph instance
	
	AudioGraphInstance * instance = audioGraphMgr.createInstance("sweetStuff6.xml");
	
	for (;;)
	{
		printf("CPU usage: %d%%\n", int(audioUpdateHandler.msecsPerSecond / 1000000.0 * 100));
		
		std::this_thread::sleep_for(std::chrono::milliseconds(1100));
	}
	
	// free the audio graph instance
	
	audioGraphMgr.free(instance, false);
	
	// shut down audio related systems

	pa.shut();
	
	audioUpdateHandler.shut();

	audioGraphMgr.shut();
	
	voiceMgr.shut();

	mutex.shut();

	return 0;
}
