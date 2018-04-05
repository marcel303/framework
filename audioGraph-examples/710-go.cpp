/*
	Copyright (C) 2017 Marcel Smit
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
#include "Debugging.h"
#include "soundmix.h"
#include <SDL2/SDL.h>

#define CHANNEL_COUNT 64

static void showHelp()
{
	printf("usage:\n");
	printf("\t710-go <filename>\n");
	printf("instantiates the audio graph given by <filename> and sends the synthesized sound to the default audio output interface.\n");
}

static void doControlValue(AudioControlValue & controlValue, const int index, const int selectedIndex, const char type, const char c)
{
	const bool isSelected = (index == selectedIndex);
	
	if (isSelected)
	{
		const float range = controlValue.max - controlValue.min;
		const float step = range / 20.f;
		
		if (c == '1')
			controlValue.desiredX = fmaxf(controlValue.desiredX - step, controlValue.min);
		if (c == '2')
			controlValue.desiredX = fminf(controlValue.desiredX + step, controlValue.max);
	}
	
	printf("%s[%c] %20s: %.2f\n",
		isSelected ? "==> " : "    ",
		type,
		controlValue.name.c_str(),
		controlValue.desiredX);
}

int main(int argc, char * argv[])
{
	const char * filename = nullptr;

	if (argc < 2)
	{
		showHelp();
		return 0;
	}

	filename = argv[1];

	if (SDL_Init(0) >= 0)
	{
		// load resources
		
		fillPcmDataCache("birds", true, false);
		fillPcmDataCache("testsounds", true, true);
		fillPcmDataCache("voice-fragments", false, false);
		
		// initialize audio related systems
		
		SDL_mutex * mutex = SDL_CreateMutex();
		Assert(mutex != nullptr);

		AudioVoiceManagerBasic voiceMgr;
		voiceMgr.init(mutex, CHANNEL_COUNT, CHANNEL_COUNT);
		voiceMgr.outputStereo = true;

		AudioGraphManager_Basic audioGraphMgr(true);
		audioGraphMgr.init(mutex, &voiceMgr);

		AudioUpdateHandler audioUpdateHandler;
		audioUpdateHandler.init(mutex, nullptr, 0);
		audioUpdateHandler.voiceMgr = &voiceMgr;
		audioUpdateHandler.audioGraphMgr = &audioGraphMgr;

		PortAudioObject pa;
		
		if (!pa.init(SAMPLE_RATE, 2, 0, AUDIO_UPDATE_SIZE, &audioUpdateHandler))
		{
			printf("failed to initialize audio output\n");
		}
		else
		{
			// create an audio graph instance
			
			AudioGraphInstance * instance = audioGraphMgr.createInstance(filename);
			
			if (instance == nullptr)
			{
				printf("failed to open %s\n", filename);
			}
			else
			{
				// todo : keyboard/console based ui to
				// - trigger events
				
				int selectedIndex = 0;
				
				int c = 0;
				
				bool stop = false;
				
				do
				{
					SDL_LockMutex(mutex);
					{
						const int count =
							audioGraphMgr.globals->controlValues.size() +
							instance->audioGraph->controlValues.size();
						
						if (count == 0)
							selectedIndex = 0;
						else
						{
							if (selectedIndex < 0)
								selectedIndex = count - 1;
							else if (selectedIndex >= count)
								selectedIndex = 0;
						}
						
						int index = 0;
						
						for (auto & controlValue : audioGraphMgr.globals->controlValues)
						{
							doControlValue(controlValue, index, selectedIndex, 'G', c);
							
							index++;
						}
						
						for (auto & controlValue : instance->audioGraph->controlValues)
						{
							doControlValue(controlValue, index, selectedIndex, 'L', c);
							
							index++;
						}
					}
					SDL_UnlockMutex(mutex);
					
					printf("CPU usage: %d%%\n", int(audioUpdateHandler.msecsPerSecond / 1000000.0 * 100));
					
					system("/bin/stty raw");
					c = getchar();
					system("/bin/stty cooked");
					
					printf("\n");
					
					if (c == 'q')
						stop = true;
					if (c == 'a')
						selectedIndex--;
					if (c == 'z')
						selectedIndex++;
				}
				while (stop == false);
			}
			
			// free the audio graph instance
			
			audioGraphMgr.free(instance, false);
		}
		
		// shut down audio related systems

		pa.shut();
		
		audioUpdateHandler.shut();

		audioGraphMgr.shut();
		
		voiceMgr.shut();

		SDL_DestroyMutex(mutex);
		mutex = nullptr;
		
		SDL_Quit();
	}

	return 0;
}
