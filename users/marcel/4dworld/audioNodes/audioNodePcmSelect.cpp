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

#include "audioNodePcmSelect.h"
#include "framework.h"

AUDIO_ENUM_TYPE(pcmSelectMode)
{
	elem("forward");
	elem("backward");
	elem("random");
}

AUDIO_NODE_TYPE(audioSourcePcmSelect, AudioNodeSourcePcmSelect)
{
	typeName = "audio.pcmSelect";
	
	in("path", "string");
	inEnum("mode", "pcmSelectMode");
	in("autoPlay", "bool");
	in("play!", "trigger");
	out("audio", "audioValue");
	out("done!", "trigger");
}

AudioNodeSourcePcmSelect::AudioNodeSourcePcmSelect()
	: AudioNodeBase()
	, currentPath()
	, fileIndex(-1)
	, samplePosition(0)
	, audioOutput()
	, doneTrigger()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Path, kAudioPlugType_String);
	addInput(kInput_Mode, kAudioPlugType_Int);
	addInput(kInput_AutoPlay, kAudioPlugType_Bool);
	addInput(kInput_Play, kAudioPlugType_Trigger);
	addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
	addOutput(kOutput_Done, kAudioPlugType_Trigger, &doneTrigger);
}

AudioNodeSourcePcmSelect::~AudioNodeSourcePcmSelect()
{
	freeFiles();
}

void AudioNodeSourcePcmSelect::freeFiles()
{
	for (auto & file : files)
	{
		delete file;
		file = nullptr;
	}

	files.clear();

	//

	fileIndex = -1;
	samplePosition = 0;
}

void AudioNodeSourcePcmSelect::nextFile(const Mode mode)
{
	if (files.empty() == false)
	{
		if (mode == kMode_Forward)
		{
			fileIndex = (fileIndex + 1 + files.size()) % files.size();
		}
		else if (mode == kMode_Backward)
		{
			fileIndex = (fileIndex - 1 + files.size()) % files.size();
		}
		else if (mode == kMode_Random)
		{
			fileIndex = rand() % files.size();
		}
		
		samplePosition = 0;
	}
}

void AudioNodeSourcePcmSelect::tick(const float dt)
{
	const char * path = getInputString(kInput_Path, "");
	const bool autoPlay = getInputBool(kInput_AutoPlay, false);
	
	if (path != currentPath)
	{
		freeFiles();
		
		//
		
		std::vector<std::string> filenames = listFiles(path, false);

		for (auto & filename : filenames)
		{
			PcmData * pcmData = new PcmData();
			
			if (pcmData->load(filename.c_str(), 0) == false || pcmData->numSamples == 0)
			{
				delete pcmData;
				pcmData = nullptr;
			}
			else
			{
				files.push_back(pcmData);
			}
		}
		
		//

		currentPath = path;
	}
	
	if (autoPlay && fileIndex == -1)
	{
		const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
		
		nextFile(mode);
	}

	if (isPassthrough)
	{
		audioOutput.setScalar(0.f);
	}
	else if (fileIndex == -1)
	{
		audioOutput.setScalar(0.f);
	}
	else
	{
		const PcmData * pcmData = files[fileIndex];

		audioOutput.setVector();

		for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
		{
			if (samplePosition >= 0 && samplePosition < pcmData->numSamples)
			{
				audioOutput.samples[i] = pcmData->samples[samplePosition];
			}
			else
			{
				audioOutput.samples[i] = 0.f;
			}

			if (samplePosition < pcmData->numSamples)
			{
				samplePosition++;

				if (samplePosition == pcmData->numSamples)
				{
					trigger(kOutput_Done);
				}
			}
		}
	}
}

void AudioNodeSourcePcmSelect::handleTrigger(const int inputSocketIndex, const AudioTriggerData & data)
{
	if (inputSocketIndex == kInput_Play)
	{
		const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
		
		nextFile(mode);
	}
}
