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
#include "audioGraphContext.h"
#include "audioGraphManager.h"
#include "audioUpdateHandler.h"
#include "audioVoiceManager.h"
#include "Debugging.h"
#include "pcmDataCache.h"

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

static void showHelp()
{
	printf("usage:\n");
	printf("\t710-go <filename>\n");
	printf("instantiates the audio graph given by <filename> and sends the synthesized sound to the default audio output interface.\n");
}

static void doControlValue(AudioControlValue & controlValue, const int index, const int selectedIndex, const char type, const int c)
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

static void doEvent(AudioGraph * audioGraph, const std::string & event, const int index, const int selectedIndex, const int c)
{
	const bool isSelected = (index == selectedIndex);
	
	if (isSelected)
	{
		if (c == ' ')
			audioGraph->triggerEvent(event.c_str());
	}
	
	printf("%sEVENT %18s\n",
		isSelected ? "==> " : "    ",
		event.c_str());
}

int main(int argc, char * argv[])
{
#if defined(CHIBI_RESOURCE_PATH)
	if (chdir(CHIBI_RESOURCE_PATH) != 0)
		return -1;
#endif

	const char * filename = nullptr;

	if (argc < 2)
	{
		showHelp();
		return 0;
	}

	filename = argv[1];

	// load resources
	
	PcmDataCache pcmDataCache;
	pcmDataCache.addPath("birds", true, false, true);
	pcmDataCache.addPath("testsounds", true, true, true);
	pcmDataCache.addPath("thegrooop", true, true, true);
	pcmDataCache.addPath("voice-fragments", false, false, true);

	// initialize audio related systems
	
	AudioMutex mutex;
	mutex.init();

	AudioVoiceManagerBasic voiceMgr;
	voiceMgr.init(&mutex, CHANNEL_COUNT);
	voiceMgr.outputStereo = true;

	AudioGraphManager_Basic audioGraphMgr(true);
	audioGraphMgr.init(&mutex, &voiceMgr);
	
	audioGraphMgr.context->addObject(&pcmDataCache, "PCM data cache");

	AudioUpdateHandler audioUpdateHandler;
	audioUpdateHandler.init(&mutex, &voiceMgr, &audioGraphMgr);

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
			int selectedIndex = 0;
			
			int c = 0;
			
			bool stop = false;
			
			do
			{
				mutex.lock();
				{
					const int count =
						audioGraphMgr.context->controlValues.size() +
						instance->audioGraph->controlValues.size() +
						instance->audioGraph->events.size();
					
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
					
					for (auto & controlValue : audioGraphMgr.context->controlValues)
					{
						doControlValue(controlValue, index, selectedIndex, 'G', c);
						
						index++;
					}
					
					for (auto & controlValue : instance->audioGraph->controlValues)
					{
						doControlValue(controlValue, index, selectedIndex, 'L', c);
						
						index++;
					}
					
					for (auto & event : instance->audioGraph->events)
					{
						doEvent(instance->audioGraph, event.name, index, selectedIndex, c);
						
						index++;
					}
				}
				mutex.unlock();
				
				printf("up/down = a/z, increment/decrement = 1/2, trigger event = SPACE, quit = q\n");
				printf("CPU usage: %d%%\n", int(audioUpdateHandler.msecsPerSecond / 1000000.0 * 100));
				
				Verify(system("/bin/stty raw") != -1);
				{
					c = getchar();
				}
				Verify(system("/bin/stty cooked") != -1);
				
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

	mutex.shut();

	return 0;
}
