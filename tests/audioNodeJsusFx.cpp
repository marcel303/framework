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
#include "audioResource.h"
#include "Log.h"

#include "jsusfx.h"
#include "jsusfx_file.h"
#include "jsusfx_gfx.h"

#include "framework.h"
#include "gfx-framework.h"
#include "jsusfx-framework.h"

#define SEARCH_PATH "/Users/thecat/atk-reaper/plugins/" // fixme : remove hard coded ATK scripts path

#define DATA_ROOT "/Users/thecat/Library/Application Support/REAPER/Data/" // fixme : remove hard coded Reaper data path

#define SLIDER_INDEX(i) (i)
#define AUDIOINPUT_INDEX(i) (sliderInputs.size() + (i))

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

#include "jsusfx_serialize.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

struct AudioResource_JsusFx : AudioResourceBase
{
	int version;
	
	JsusFxSerializationData serializationData;
	
	AudioResource_JsusFx()
		: version(0)
		, serializationData()
	{
	}
	
	virtual void save(tinyxml2::XMLPrinter * printer) override
	{
		tinyxml2::XMLPrinter & p = *printer;
		
		if (!serializationData.sliders.empty())
		{
			p.OpenElement("sliders");
			{
				for (auto & slider : serializationData.sliders)
				{
					p.OpenElement("slider");
					{
						p.PushAttribute("index", slider.index);
						p.PushAttribute("value", slider.value);
					}
					p.CloseElement();
				}
			}
			p.CloseElement();
		}
		
		if (!serializationData.vars.empty())
		{
			p.OpenElement("vars");
			{
				for (auto & var : serializationData.vars)
				{
					p.OpenElement("var");
					{
						p.PushAttribute("value", var);
					}
					p.CloseElement();
				}
			}
			p.CloseElement();
		}
	}
	
	virtual void load(tinyxml2::XMLElement * xml_effect) override
	{
		auto xml_sliders = xml_effect->FirstChildElement("sliders");
	
		if (xml_sliders != nullptr)
		{
			for (auto xml_slider = xml_sliders->FirstChildElement("slider"); xml_slider != nullptr; xml_slider = xml_slider->NextSiblingElement("slider"))
			{
				JsusFxSerializationData::Slider slider;
				
				slider.index = intAttrib(xml_slider, "index", -1);
				slider.value = floatAttrib(xml_slider, "value", 0.f);
				
				serializationData.sliders.push_back(slider);
			}
		}
	
		auto xml_vars = xml_effect->FirstChildElement("vars");
	
		if (xml_vars != nullptr)
		{
			for (auto xml_var = xml_vars->FirstChildElement("var"); xml_var != nullptr; xml_var = xml_var->NextSiblingElement("var"))
			{
				const float value = floatAttrib(xml_var, "value", 0.f);
				
				serializationData.vars.push_back(value);
			}
		}
		
		//
		
		version++;
	}
};

AUDIO_RESOURCE_TYPE(AudioResource_JsusFx, "jsusfx");

//

struct ResourceEditor_JsusFx : GraphEdit_ResourceEditorBase
{
	AudioResource_JsusFx * resource = nullptr;
	
	// we need to create a jsusfx instance to visually edit the resource
	JsusFxPathLibrary_Basic pathLibrary;
	JsusFxFileAPI_Basic fileAPI;
	JsusFx_Framework jsusFx;
	JsusFxGfx_Framework gfx;
	bool jsusFxIsValid;
	
	ResourceEditor_JsusFx(const char * filename)
		: GraphEdit_ResourceEditorBase(400, 400)
		, resource(nullptr)
		, pathLibrary(DATA_ROOT)
		, fileAPI()
		, jsusFx(pathLibrary)
		, gfx(jsusFx)
		, jsusFxIsValid(false)
	{
		JsusFx::init();
		
		// setup interfaces
		
		pathLibrary.addSearchPath(SEARCH_PATH);
		
		fileAPI.init(jsusFx.m_vm);
		jsusFx.fileAPI = &fileAPI;
		
		gfx.init(jsusFx.m_vm);
		jsusFx.gfx = &gfx;
		
		// compile the effect
		
		jsusFxIsValid = jsusFx.compile(
			pathLibrary,
			filename,
			JsusFx::kCompileFlag_CompileGraphicsSection |
			JsusFx::kCompileFlag_CompileSerializeSection);
		
		jsusFx.prepare(SAMPLE_RATE, AUDIO_UPDATE_SIZE);
	}
	
	virtual ~ResourceEditor_JsusFx()
	{
		freeAudioNodeResource(resource);
		Assert(resource == nullptr);
	}
	
	virtual void afterSizeChanged() override
	{
	}
	
	virtual void afterPositionChanged() override
	{
	}
	
	virtual bool tick(const float dt, const bool inputIsCaptured) override
	{
		if (jsusFxIsValid)
		{
			gfx.setup(nullptr, sx, sy, mouse.x, mouse.y, inputIsCaptured == false);
			
			jsusFx.process(nullptr, nullptr, AUDIO_UPDATE_SIZE, 0, 0);
		}
		
		if (jsusFxIsValid && resource != nullptr)
		{
			// it's difficult to tell if the user interacted with the effect's ui and made changes. so we just
			// serialize the effect each tick and see if it's different from the current serialization data
			
			JsusFxSerializationData serializationData;
			
			JsusFxSerializer_Basic serializer(serializationData);
		
			if (jsusFx.serialize(serializer, true))
			{
				if (serializationData != resource->serializationData)
				{
					// it changed! update the resource
					
					lock();
					{
						resource->serializationData = serializationData;
						
						resource->version++;
					}
					unlock();
				}
			}
		}
		
		return false;
	}
	
	virtual void draw() const override
	{
		if (jsusFxIsValid)
		{
			const_cast<JsusFx_Framework&>(jsusFx).draw();
		}
	}
	
	virtual void setResource(const GraphNode & node, const char * type, const char * name) override
	{
		Assert(resource == nullptr);
		
		if (createAudioNodeResource(node, type, name, resource))
		{
			if (jsusFxIsValid)
			{
				// load the initial data into the effect
				
				JsusFxSerializer_Basic serializer(resource->serializationData);
				
				jsusFx.serialize(serializer, false);
			}
		}
	}
	
	virtual bool serializeResource(std::string & text) const override
	{
		if (resource == nullptr)
		{
			return false;
		}
		else
		{
			tinyxml2::XMLPrinter p;
			p.OpenElement("value");
			{
				resource->save(&p);
			}
			p.CloseElement();
			
			text = p.CStr();
			
			return true;
		}
	}
};

//

#include "Path.h"
#include "StringEx.h"

struct AudioNodeTypeRegistration_JsusFx : AudioNodeTypeRegistration
{
	std::string filename;
	
	std::vector<AudioNodeJsusFx::SliderInput> sliderInputs;
	
	int numInputs = 0;
	int numOutputs = 0;
	
	void initFromJsusFx(const JsusFx_Framework & jsusFx, const char * _filename)
	{
		filename = _filename;
		
		typeName = String::FormatC("jsusfx.%s", jsusFx.desc);
		
		// add slider inputs
		
		for (int i = 0; i < jsusFx.kMaxSliders; ++i)
		{
			auto & slider = jsusFx.sliders[i];
			
			if (!slider.exists)
				continue;
			
			AudioNodeJsusFx::SliderInput sliderInput;
			sliderInput.name = slider.name;
			sliderInput.defaultValue = slider.def;
			sliderInput.sliderIndex = i;
			sliderInput.socketIndex = inputs.size();
			
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
		
		// add audio inputs
		
		for (int i = 0; i < jsusFx.numInputs; ++i)
		{
			const std::string name = String::FormatC("in%d", i + 1);
			
			in(name.c_str(), "audioValue");
		}
		
		numInputs = jsusFx.numInputs;
		
		// add audio outputs
		
		for (int i = 0; i < jsusFx.numOutputs; ++i)
		{
			const std::string name = String::FormatC("out%d", i + 1);
			
			out(name.c_str(), "audioValue");
		}
		
		numOutputs = jsusFx.numOutputs;
		
		resourceTypeName = "jsusfx";
	}
};

static AudioNodeBase * createJsusFxNode(const AudioNodeTypeRegistration_JsusFx * r)
{
	AudioNodeJsusFx * fx = new AudioNodeJsusFx();
	
	fx->resizeSockets(r->sliderInputs.size() + r->numInputs, r->numOutputs);
	
	fx->filename = r->filename;
	fx->sliderInputs = r->sliderInputs;
	fx->numAudioInputs = r->numInputs;
	fx->numAudioOutputs = r->numOutputs;
	
	// add input sockets
	
	{
		int inputIndex = 0;
		
		for (int i = 0; i < r->sliderInputs.size(); ++i)
			fx->addInput(inputIndex++, kAudioPlugType_FloatVec);
		
		for (int i = 0; i < r->numInputs; ++i)
			fx->addInput(inputIndex++, kAudioPlugType_FloatVec);
	}
	
	// add output sockets
	
	{
		fx->audioOutputs.resize(r->numOutputs);
		
		int outputIndex = 0;
		
		for (int i = 0; i < r->numOutputs; ++i)
			fx->addOutput(outputIndex++, kAudioPlugType_FloatVec, &fx->audioOutputs[i]);
	}
	
	return fx;
}

void createJsusFxAudioNodes()
{
	JsusFx::init();
	
	auto filenames = listFiles(SEARCH_PATH, true);
	
	JsusFxPathLibrary_Basic pathLibrary(DATA_ROOT);
	
	for (auto & filename : filenames)
	{
		auto extension = Path::GetExtension(filename, true);
		
		if (extension != "" && extension != "jsfx")
			continue;
		
		JsusFx_Framework jsusFx(pathLibrary);
		
		if (!jsusFx.readHeader(pathLibrary, filename))
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
		
		r->createResourceEditor = [](void * data) -> GraphEdit_ResourceEditorBase*
		{
			const AudioNodeTypeRegistration_JsusFx * r = (AudioNodeTypeRegistration_JsusFx*)data;
			
			return new ResourceEditor_JsusFx(r->filename.c_str());
		};
	}
}

//

AudioNodeJsusFx::AudioNodeJsusFx()
	: AudioNodeBase()
	, filename()
	, currentFilename()
	, numAudioInputs(0)
	, numAudioOutputs(0)
	, audioOutputs()
	, pathLibrary(nullptr)
	, jsusFx(nullptr)
	, jsusFxIsValid(false)
	, jsusFx_fileAPI(nullptr)
	, jsusFx_gfx(nullptr)
	, resource(nullptr)
	, resourceVersion(0)
{
}

AudioNodeJsusFx::~AudioNodeJsusFx()
{
	free();
	
	freeAudioNodeResource(resource);
	Assert(resource == nullptr);
}

void AudioNodeJsusFx::load(const char * filename)
{
	free();
	
	//
	
	Assert(pathLibrary == nullptr);
	pathLibrary = new JsusFxPathLibrary_Basic(DATA_ROOT);
	pathLibrary->addSearchPath(SEARCH_PATH);
	
	Assert(jsusFx == nullptr);
	jsusFx = new JsusFx_Framework(*pathLibrary);
	
	Assert(jsusFx_fileAPI == nullptr);
	jsusFx_fileAPI = new JsusFxFileAPI_Basic();
	jsusFx_fileAPI->init(jsusFx->m_vm);
	jsusFx->fileAPI = jsusFx_fileAPI;
	
	Assert(jsusFx_gfx == nullptr);
	jsusFx_gfx = new JsusFxGfx_Framework(*jsusFx);
	jsusFx_gfx->init(jsusFx->m_vm);
	jsusFx->gfx = jsusFx_gfx;
	
	//
	
	Assert(jsusFxIsValid == false);
	
	jsusFxIsValid = jsusFx->compile(*pathLibrary, filename, JsusFx::kCompileFlag_CompileGraphicsSection | JsusFx::kCompileFlag_CompileSerializeSection);
	
	if (jsusFxIsValid)
	{
		jsusFx->prepare(SAMPLE_RATE, AUDIO_UPDATE_SIZE);
	}
}

void AudioNodeJsusFx::free()
{
	jsusFxIsValid = false;
	
	delete jsusFx_gfx;
	jsusFx_gfx = nullptr;
	
	if (jsusFx != nullptr)
		jsusFx->gfx = nullptr;
	
	delete jsusFx_fileAPI;
	jsusFx_fileAPI = nullptr;
	
	if (jsusFx != nullptr)
		jsusFx->fileAPI = nullptr;
	
	delete jsusFx;
	jsusFx = nullptr;
	
	delete pathLibrary;
	pathLibrary = nullptr;
}

void AudioNodeJsusFx::clearOutputs()
{
	for (auto & audioOutput : audioOutputs)
		audioOutput.setZero();
}

void AudioNodeJsusFx::init(const GraphNode & node)
{
	Assert(resource == nullptr);
	createAudioNodeResource(node, "jsusfx", "editorData", resource);
	
	if (isPassthrough)
		return;
	
	Assert(!filename.empty());
	if (filename != currentFilename)
	{
		load(filename.c_str());
		currentFilename = filename;
	}
}

void AudioNodeJsusFx::tick(const float dt)
{
	if (isPassthrough)
	{
		clearOutputs();
		return;
	}
	
	Assert(!filename.empty());
	if (filename != currentFilename)
	{
		load(filename.c_str());
		currentFilename = filename;
	}
	
	if (resourceVersion != resource->version)
	{
		lock();
		{
			resourceVersion = resource->version;
			
			if (jsusFxIsValid)
			{
				JsusFxSerializer_Basic serializer(resource->serializationData);
				
				jsusFx->serialize(serializer, false);
			}
		}
		unlock();
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
		
		AudioFloat defaultValue;
		
		for (auto & sliderInput : sliderInputs)
		{
			auto input = tryGetInput(sliderInput.socketIndex);
			
			Assert(input != nullptr);
			if (input == nullptr || !input->isConnected())
				continue;
			
			const float value = input->getAudioFloat().getMean();
			
			Assert(jsusFx->sliders[sliderInput.sliderIndex].exists);
			
			jsusFx->moveSlider(sliderInput.sliderIndex, value);
		}
		
		// execute script
		
		const float ** input = (const float**)alloca(numAudioInputs * sizeof(float*));
		
		for (int i = 0; i < numAudioInputs; ++i)
		{
			const AudioFloat * audioInput = getInputAudioFloat(AUDIOINPUT_INDEX(i), &AudioFloat::Zero);
			audioInput->expand();
			
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

void AudioNodeJsusFx::getDescription(AudioNodeDescription & d)
{
	for (int i = 0; i < JsusFx::kMaxSliders; ++i)
	{
		auto & slider = jsusFx->sliders[i];
		
		if (!slider.exists)
			continue;
		
		d.add("slider %s. index=%d, default=%f, increment=%f, desc=%s", slider.name, i, slider.def, slider.inc, slider.desc);
	}
	
	for (auto & sliderInput : sliderInputs)
	{
		d.add("registered slider input %s. socket_index=%d, slider_index=%d",
			sliderInput.name.c_str(),
			sliderInput.socketIndex,
			sliderInput.sliderIndex);
	}
}

#undef SEARCH_PATH
#undef DATA_ROOT
#undef SLIDER_INDEX
#undef AUDIOINPUT_INDEX
