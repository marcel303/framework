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
#include "audioNodePcmSelect.h"
#include "framework.h" // listFiles
#include "soundmix.h" // PcmData

AUDIO_ENUM_TYPE(pcmSelectMode)
{
	elem("forward");
	elem("backward");
	elem("random");
}

AUDIO_NODE_TYPE(AudioNodeSourcePcmSelect)
{
	typeName = "audio.pcmSelect";
	
	in("path", "string");
	inEnum("mode", "pcmSelectMode");
	in("gain", "audioValue", "1");
	in("autoPlay", "bool");
	in("loopCount", "int");
	in("play!", "trigger");
	in("stop!", "trigger");
	out("audio", "audioValue");
	out("loop!", "trigger");
	out("done!", "trigger");
}

AudioNodeSourcePcmSelect::AudioNodeSourcePcmSelect()
	: AudioNodeBase()
	, currentPath()
	, fileIndex(-1)
	, samplePosition(0)
	, loopCount(0)
	, audioOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Path, kAudioPlugType_String);
	addInput(kInput_Mode, kAudioPlugType_Int);
	addInput(kInput_Gain, kAudioPlugType_FloatVec);
	addInput(kInput_AutoPlay, kAudioPlugType_Bool);
	addInput(kInput_MaxLoopCount, kAudioPlugType_Int);
	addInput(kInput_Play, kAudioPlugType_Trigger);
	addInput(kInput_Stop, kAudioPlugType_Trigger);
	addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);
	addOutput(kOutput_Loop, kAudioPlugType_Trigger, this);
	addOutput(kOutput_Done, kAudioPlugType_Trigger, this);
}

AudioNodeSourcePcmSelect::~AudioNodeSourcePcmSelect()
{
	freeFiles();
}

void AudioNodeSourcePcmSelect::freeFiles()
{
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
	const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
	const AudioFloat * gain = getInputAudioFloat(kInput_Gain, &AudioFloat::One);
	const bool autoPlay = getInputBool(kInput_AutoPlay, false);
	const int maxLoopCount = getInputInt(kInput_MaxLoopCount, 0);
	
	if (path != currentPath)
	{
		freeFiles();
		
		//
		
		const std::vector<std::string> filenames = listFiles(path, false);

		for (auto & filename : filenames)
		{
			const PcmData * pcmData = getPcmData(filename.c_str());
			
			if (pcmData != nullptr && pcmData->numSamples > 0)
			{
				files.push_back(pcmData);
				
				this->filenames.push_back(filename);
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
	else if (fileIndex == -1 || files[fileIndex]->numSamples == 0)
	{
		audioOutput.setScalar(0.f);
	}
	else
	{
		const PcmData * pcmData = files[fileIndex];
		
		audioOutput.setVector();
		
		int left = AUDIO_UPDATE_SIZE;
		int done = 0;
		
		while (left != 0)
		{
			const int todo = std::min(left, pcmData->numSamples - samplePosition);
		
			memcpy(audioOutput.samples + done, pcmData->samples + samplePosition, todo * sizeof(float));
			
			samplePosition += todo;
			
			if (samplePosition == pcmData->numSamples)
			{
				if (maxLoopCount < 0 || loopCount < maxLoopCount)
				{
					loopCount++;
					
					nextFile(mode);
					
					trigger(kOutput_Loop);
				}
				else
				{
					trigger(kOutput_Done);
					
					break;
				}
			}
			
			left -= todo;
			done += todo;
		}
		
		if (left != 0)
		{
			memset(audioOutput.samples + done, 0, left * sizeof(float));
		}
		
		audioOutput.mul(*gain);
	}
}

void AudioNodeSourcePcmSelect::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_Play)
	{
		const Mode mode = (Mode)getInputInt(kInput_Mode, 0);
		
		nextFile(mode);
	}
	else if (inputSocketIndex == kInput_Stop)
	{
		fileIndex = -1;
		samplePosition = 0;
		
		trigger(kOutput_Done);
	}
}

void AudioNodeSourcePcmSelect::getDescription(AudioNodeDescription & d)
{
	if (fileIndex < 0)
		d.add("not playing");
	else
		d.add("file: %s", filenames[fileIndex].c_str());
}
