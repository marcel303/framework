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
#include "audioNodeJsusFx.h"
#include "Log.h"

#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"

#include "framework.h"
#include "gfx-framework.h"
#include "jsusfx-framework.h"

#define SEARCH_PATH "/Users/thecat/atk-reaper/plugins/"

#define DATA_ROOT "/Users/thecat/Library/Application Support/REAPER/Data/"

AUDIO_NODE_TYPE(jsusfx, AudioNodeJsusFx)
{
	typeName = "jsusfx";
	
	in("file", "string");
	in("input1", "audioValue");
	in("input2", "audioValue");
	in("slider1", "audioValue");
	in("slider2", "audioValue");
	in("slider3", "audioValue");
	in("slider4", "audioValue");
	out("audio1", "audioValue");
	out("audio2", "audioValue");
}

AudioNodeJsusFx::AudioNodeJsusFx()
	: AudioNodeBase()
	, audioOutput1()
	, audioOutput2()
	, pathLibrary(nullptr)
	, jsusFx(nullptr)
	, jsusFxIsValid(false)
	, jsusFx_fileAPI(nullptr)
	, jsusFx_gfx(nullptr)
	, currentFilename()
	, hasFocus(false)
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Filename, kAudioPlugType_String);
	addInput(kInput_Input1, kAudioPlugType_FloatVec);
	addInput(kInput_Input2, kAudioPlugType_FloatVec);
	addInput(kInput_Slider1, kAudioPlugType_FloatVec);
	addInput(kInput_Slider2, kAudioPlugType_FloatVec);
	addInput(kInput_Slider3, kAudioPlugType_FloatVec);
	addInput(kInput_Slider4, kAudioPlugType_FloatVec);
	addOutput(kOutput_Audio1, kAudioPlugType_FloatVec, &audioOutput1);
	addOutput(kOutput_Audio2, kAudioPlugType_FloatVec, &audioOutput2);
	
	static bool isInitialized = false;
	
	if (isInitialized == false)
	{
		isInitialized = true;
		JsusFx::init();
	}
	
	pathLibrary = new JsusFxPathLibrary_Basic(DATA_ROOT);
	pathLibrary->addSearchPath(SEARCH_PATH);
	
	jsusFx = new JsusFx_Framework(*pathLibrary);
	
	jsusFx_fileAPI = new JsusFxFileAPI_Basic();
	jsusFx_fileAPI->init(jsusFx->m_vm);
	jsusFx->fileAPI = jsusFx_fileAPI;
	
	jsusFx_gfx = new JsusFxGfx_Framework(*jsusFx);
	jsusFx_gfx->init(jsusFx->m_vm);
	jsusFx->gfx = jsusFx_gfx;
}

AudioNodeJsusFx::~AudioNodeJsusFx()
{
	free();
}

void AudioNodeJsusFx::load(const char * filename)
{
	jsusFxIsValid = jsusFx->compile(*pathLibrary, filename);
	
	if (jsusFxIsValid)
	{
		jsusFx->prepare(SAMPLE_RATE, AUDIO_UPDATE_SIZE);
	}
}

void AudioNodeJsusFx::free()
{
	delete jsusFx_gfx;
	jsusFx_gfx = nullptr;
	jsusFx->gfx = nullptr;
	
	delete jsusFx_fileAPI;
	jsusFx_fileAPI = nullptr;
	jsusFx->fileAPI = nullptr;
	
	delete jsusFx;
	jsusFx = nullptr;
	
	delete pathLibrary;
	pathLibrary = nullptr;
	
	currentFilename.clear();
	
	hasFocus = false;
}

void AudioNodeJsusFx::clearOutputs()
{
	audioOutput1.setZero();
	audioOutput2.setZero();
}

void AudioNodeJsusFx::tick(const float dt)
{
	// todo : passthrough support

	const char * filename = getInputString(kInput_Filename, nullptr);
	const AudioFloat * input1 = getInputAudioFloat(kInput_Input1, &AudioFloat::Zero);
	const AudioFloat * input2 = getInputAudioFloat(kInput_Input2, &AudioFloat::Zero);
	const float slider1 = getInputAudioFloat(kInput_Slider1, &AudioFloat::Zero)->getMean();
	const float slider2 = getInputAudioFloat(kInput_Slider2, &AudioFloat::Zero)->getMean();
	const float slider3 = getInputAudioFloat(kInput_Slider3, &AudioFloat::Zero)->getMean();
	const float slider4 = getInputAudioFloat(kInput_Slider4, &AudioFloat::Zero)->getMean();

	if (isPassthrough || filename == nullptr)
	{
		currentFilename.clear();
		clearOutputs();
		return;
	}
	
	// reload script if filename changed

	if (filename != currentFilename)
	{
		currentFilename = filename;
		load(filename);
	}
	
	if (jsusFxIsValid == false)
	{
		clearOutputs();
	}
	else
	{
		input1->expand();
		input2->expand();
		
		audioOutput1.setVector();
		audioOutput2.setVector();
		
		// update slider values
		
		const float sliderValues[4] =
		{
			slider1,
			slider2,
			slider3,
			slider4
		};
		
		for (int i = 0; i < 4; ++i)
		{
			jsusFx->moveSlider(i + 1, sliderValues[i]);
		}
		
		// execute script
		
		const float * input[2] =
		{
			input1->samples,
			input2->samples
		};
		
		float * output[2] =
		{
			audioOutput1.samples,
			audioOutput2.samples
		};
		
		if (!jsusFx->process(input, output, AUDIO_UPDATE_SIZE, 2, 2))
		{
			audioOutput1.setZero();
			audioOutput2.setZero();
		}
	}
}

bool AudioNodeJsusFx::tickEditor(const int x, const int y, int & sx, int & sy, bool & inputIsCaptured)
{
	if (jsusFx == nullptr)
	{
		hasFocus = false;
	}
	else
	{
		sx = jsusFx->gfx_w;
		sy = jsusFx->gfx_h;
		
		if (inputIsCaptured)
			hasFocus = false;
		else
		{
			const int mouseX = mouse.x - x;
			const int mouseY = mouse.y - y;
			
			const bool isInside =
				mouseX >= 0 && mouseX < sx &&
				mouseY >= 0 && mouseY < sy;
			
			if (mouse.wentDown(BUTTON_LEFT))
			{
				if (isInside)
					hasFocus = true;
				else
					hasFocus = false;
			}
		}
	}
	
	inputIsCaptured |= hasFocus;
	
	return true;
}

void AudioNodeJsusFx::drawEditor(Surface * surface, const int x, const int y, const int sx, const int sy)
{
	jsusFx_gfx->setup(surface, sx, sy, mouse.x - x, mouse.y - y, true);
	
	jsusFx->draw();
	
	// update input values

	for (int i = 1; i < JsusFx::kMaxSliders; ++i)
	{
		auto & slider = jsusFx->sliders[i];
		
		if (slider.exists)
		{
			const int inputIndex = i - 1;
			
			if (inputIndex >= 0 && inputIndex < 4)
			{
				AudioPlug * input = tryGetInput(kInput_Slider1 + inputIndex);
				
				if (input->floatArray.immediateValue == nullptr)
					g_currentAudioGraph->connectToInputLiteral(*input, "");
				
				input->floatArray.immediateValue->setScalar(slider.getValue());
			}
		}
	}
}
