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

#define SEARCH_PATH "/Users/thecat/atk-reaper/plugins/" // fixme : remove hard coded ATK scripts path

#define DATA_ROOT "/Users/thecat/Library/Application Support/REAPER/Data/" // fixme : remove hard coded Reaper data path

#define DYN_OFFSET 1 // todo : remove

//

extern SDL_mutex * g_vfxAudioMutex; // fixme : remove this dependency

static void lock()
{
	const int r = SDL_LockMutex(g_vfxAudioMutex); // fixme : mutex lock around draw code is a horrible idea!
	Assert(r == 0);
}

static void unlock()
{
	const int r = SDL_UnlockMutex(g_vfxAudioMutex); // fixme : mutex lock around draw code is a horrible idea!
	Assert(r == 0);
}

//

#include "Path.h"
#include "StringEx.h"

struct AudioNodeTypeRegistration_JsusFx : AudioNodeTypeRegistration
{
	struct SliderInput
	{
		std::string name;
		float defaultValue; // todo : honour default value when fetching slider values
	};
	
	std::string filename;
	
	std::vector<SliderInput> sliderInputs;
	
	int numInputs = 0;
	int numOutputs = 0;
	
	void initFromJsusFx(const JsusFx_Framework & jsusFx, const char * _filename)
	{
		filename = _filename;
		
		typeName = String::FormatC("jsusfx.%s", jsusFx.desc);
		
		in("filename", "string");
		
		// add audio inputs
		
		for (int i = 0; i < jsusFx.numInputs; ++i)
		{
			const std::string name = String::FormatC("in%d", i + 1);
			
			in(name.c_str(), "audioValue");
		}
		
		numInputs = jsusFx.numInputs;
		
		// add slider inputs
		
		for (int i = 0; i < jsusFx.kMaxSliders; ++i)
		{
			auto & slider = jsusFx.sliders[i];
			
			if (!slider.exists)
				continue;
			
			SliderInput sliderInput;
			sliderInput.name = slider.name;
			sliderInput.defaultValue = slider.def;
			
			char defaultString[64];
			sprintf_s(defaultString, sizeof(defaultString), "%g", slider.def);
			
			if (slider.isEnum)
			{
				// add enumeration type registration with a name unique to this node type ..
				
				const std::string enumName = String::FormatC("%s_%s", typeName.c_str(), slider.name);
				
				AudioEnumTypeRegistration * e = new AudioEnumTypeRegistration();
				e->enumName = enumName;
				for (int i = 0; i < slider.enumNames.size(); ++i)
					e->elem(slider.enumNames[i].c_str());
				
				// .. and add the enum input itself
				
				inEnum(sliderInput.name.c_str(), enumName.c_str(), defaultString, slider.desc);
			}
			else
			{
				in(sliderInput.name.c_str(), "audioValue", defaultString, slider.desc);
			}
			
			sliderInputs.push_back(sliderInput);
		}
		
		// add audio outputs
		
		for (int i = 0; i < jsusFx.numOutputs; ++i)
		{
			const std::string name = String::FormatC("out%d", i + 1);
			
			out(name.c_str(), "audioValue");
		}
		
		numOutputs = jsusFx.numOutputs;
	}
};

static AudioNodeBase * createJsusFxNode(const AudioNodeTypeRegistration_JsusFx * r)
{
	AudioNodeJsusFx * fx = new AudioNodeJsusFx(true);
	
	fx->resizeSockets(1 + r->numInputs + r->sliderInputs.size(), r->numOutputs);
	
	fx->numAudioInputs = r->numInputs;
	fx->numSliderInputs = r->sliderInputs.size();
	fx->numAudioOutputs = r->numOutputs;
	
	// add input sockets
	
	{
		int inputIndex = 0;
		
		fx->addInput(inputIndex++, kAudioPlugType_String); // filename. todo : remove
		
		for (int i = 0; i < r->numInputs; ++i)
			fx->addInput(inputIndex++, kAudioPlugType_FloatVec);
		
		for (int i = 0; i < r->sliderInputs.size(); ++i)
			fx->addInput(inputIndex++, kAudioPlugType_FloatVec);
	}
	
	// add output sockets
	
	{
		fx->audioOutputs.resize(r->numOutputs);
		
		int outputIndex = 0;
		
		for (int i = 0; i < r->numOutputs; ++i)
			fx->addOutput(outputIndex++, kAudioPlugType_FloatVec, &fx->audioOutputs[i]);
	}
	
	fx->load(r->filename.c_str());
	
	return fx;
}

void createJsusFxAudioNodes()
{
	JsusFx::init();
	
	auto filenames = listFiles(SEARCH_PATH, true);
	
	JsusFxPathLibrary_Basic pathLibrary(DATA_ROOT);
	JsusFxFileAPI_Basic fileAPI;
	JsusFxGfx gfxAPI;
	
	for (auto & filename : filenames)
	{
		auto extension = Path::GetExtension(filename, true);
		
		if (extension != "" && extension != "jsfx")
			continue;
		
		JsusFx_Framework jsusFx(pathLibrary);
		
		fileAPI.init(jsusFx.m_vm);
		jsusFx.fileAPI = &fileAPI;
		
		gfxAPI.init(jsusFx.m_vm);
		jsusFx.gfx = &gfxAPI;
		
		if (!jsusFx.compile(pathLibrary, filename))
			continue;
		
		if (jsusFx.desc[0] == 0)
			continue;
		
		AudioNodeTypeRegistration_JsusFx * r = new AudioNodeTypeRegistration_JsusFx();
		r->initFromJsusFx(jsusFx, filename.c_str());
		
		r->create = [](void * data)
		{
			const AudioNodeTypeRegistration_JsusFx * r = (AudioNodeTypeRegistration_JsusFx*)data;
			
			AudioNodeBase * node = createJsusFxNode(r);
			
			return node;
		};
		
		r->createData = r;
	}
}

//

AUDIO_NODE_TYPE(jsusfx, AudioNodeJsusFx)
{
	typeName = "jsusfx";
	
	in("file", "string");
	in("input1", "audioValue");
	in("input2", "audioValue");
	in("input3", "audioValue");
	in("input4", "audioValue");
	in("slider1", "audioValue");
	in("slider2", "audioValue");
	in("slider3", "audioValue");
	in("slider4", "audioValue");
	out("audio1", "audioValue");
	out("audio2", "audioValue");
	out("audio3", "audioValue");
	out("audio4", "audioValue");
}

AudioNodeJsusFx::AudioNodeJsusFx(const bool _preInitialized)
	: AudioNodeBase()
	, preInitialized(false)
	, numAudioInputs(0)
	, numSliderInputs(0)
	, numAudioOutputs(0)
	, audioOutputs()
	, pathLibrary(nullptr)
	, jsusFx(nullptr)
	, jsusFxIsValid(false)
	, jsusFx_fileAPI(nullptr)
	, jsusFx_gfx(nullptr)
	, currentFilename()
	, hasFocus(false)
{
	preInitialized = _preInitialized;
	
	if (preInitialized == false)
	{
		numAudioInputs = 4;
		numSliderInputs = 4;
		numAudioOutputs = 4;
		
		audioOutputs.resize(4);
		
		resizeSockets(kInput_COUNT, kOutput_COUNT);
		addInput(kInput_Filename, kAudioPlugType_String);
		addInput(kInput_Input1, kAudioPlugType_FloatVec);
		addInput(kInput_Input2, kAudioPlugType_FloatVec);
		addInput(kInput_Input3, kAudioPlugType_FloatVec);
		addInput(kInput_Input4, kAudioPlugType_FloatVec);
		addInput(kInput_Slider1, kAudioPlugType_FloatVec);
		addInput(kInput_Slider2, kAudioPlugType_FloatVec);
		addInput(kInput_Slider3, kAudioPlugType_FloatVec);
		addInput(kInput_Slider4, kAudioPlugType_FloatVec);
		addOutput(kOutput_Audio1, kAudioPlugType_FloatVec, &audioOutputs[0]);
		addOutput(kOutput_Audio2, kAudioPlugType_FloatVec, &audioOutputs[1]);
		addOutput(kOutput_Audio3, kAudioPlugType_FloatVec, &audioOutputs[2]);
		addOutput(kOutput_Audio4, kAudioPlugType_FloatVec, &audioOutputs[3]);
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
	for (auto & audioOutput : audioOutputs)
		audioOutput.setZero();
}

bool AudioNodeJsusFx::isSliderConnected(const int index) const
{
	if (index < 0 || index >= numSliderInputs)
		return false;
	
	const AudioPlug * input = tryGetInput(DYN_OFFSET + numAudioInputs + index);
	
	if (input->floatArray.elems.empty())
		return false;
	else
		return true;
}

void AudioNodeJsusFx::updateImmediateValues()
{
	// update sliders with input values. only literals are processed here. the rule
	// is connected input sockets are left untouched and are updated by the audio graph,
	// and literals are edited/updated through the graph editor when changing literals or
	// through this node editor by the @gfx section

	for (int i = 0; i < numSliderInputs; ++i)
	{
		if (isSliderConnected(i))
			continue;
		
		auto & slider = jsusFx->sliders[i + 1];
		
		if (!slider.exists)
			continue;
		
		AudioPlug * input = tryGetInput(DYN_OFFSET + numAudioInputs + i);
		
		if (input->floatArray.immediateValue == nullptr)
		{
			g_currentAudioGraph->connectToInputLiteral(*input, "");
		}
		
		jsusFx->moveSlider(i + 1, input->getAudioFloat().getMean());
	}
}

void AudioNodeJsusFx::init(const GraphNode & node)
{
	const char * filename = getInputString(kInput_Filename, nullptr);
	
	if (isPassthrough || filename == nullptr)
		return;
	
	// reload script if filename changed

	if (filename != currentFilename)
	{
		currentFilename = filename;
		load(filename);
	}
	
	updateImmediateValues();
}

void AudioNodeJsusFx::tick(const float dt)
{
	if (preInitialized)
	{
		// todo : passthrough support
		
		if (isPassthrough)
		{
			//currentFilename.clear();
			clearOutputs();
			return;
		}
	}
	else
	{
		// todo : passthrough support

		const char * filename = getInputString(kInput_Filename, nullptr);
		
		if (isPassthrough || filename == nullptr)
		{
			//currentFilename.clear();
			clearOutputs();
			return;
		}
		
		// reload script if filename changed

		if (filename != currentFilename)
		{
			currentFilename = filename;
			load(filename);
		}
	}
	
	if (jsusFxIsValid == false)
	{
		clearOutputs();
	}
	else
	{
		// update slider values
		
	// fixme : automated slider changes should directly set the slider values
	//         moveSlider / @slider should not be invoked. only when the user
	//         changes the slider through the Reaper slider UI
	// at least, this is what the Reaper documentation and forum posts are telling me..
	// but does this make sense? it wouldn't work with the ATK code I've seen for instance..
	// how does Reaper handle automation events?
		
		for (int i = 0; i < numSliderInputs; ++i)
		{
			if (!isSliderConnected(i))
				continue;
			
			const float value = getInputAudioFloat(DYN_OFFSET + numAudioInputs + i, &AudioFloat::Zero)->getMean();
			
			jsusFx->moveSlider(i + 1, value);
		}
		
		// execute script
		
		const float ** input = (const float**)alloca(numAudioInputs * sizeof(float*));
		
		for (int i = 0; i < numAudioInputs; ++i)
		{
			const AudioFloat * audioInput = getInputAudioFloat(DYN_OFFSET + i, &AudioFloat::Zero);
			
			input[i] = audioInput->samples;
		}
		
		float ** output = (float**)alloca(numAudioOutputs * sizeof(float*));
		
		for (int i = 0; i < numAudioOutputs; ++i)
		{
			audioOutputs[i].setVector();
			
			output[i] = audioOutputs[i].samples;
		}
		
		if (!jsusFx->process(input, output, AUDIO_UPDATE_SIZE, numAudioInputs, numAudioOutputs))
		{
			clearOutputs();
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
#if 1
	lock();
	{
		updateImmediateValues();
	}
	unlock();
#endif

	jsusFx_gfx->setup(surface, sx, sy, mouse.x - x, mouse.y - y, true);
	
	jsusFx->draw();
	
	lock();
	{
		// update input values with slider values
		
		for (int i = 0; i < numSliderInputs; ++i)
		{
			if (isSliderConnected(i))
				continue;
			
			auto & slider = jsusFx->sliders[i + 1];
			
			if (!slider.exists)
				continue;
			
			AudioPlug * input = tryGetInput(DYN_OFFSET + numAudioInputs + i);
			
			Assert(input->floatArray.immediateValue != nullptr);
			
			const float value = slider.getValue();
			
			input->floatArray.immediateValue->setScalar(value);
		}
	}
	unlock();
}

#undef DYN_OFFSET // todo : remove
