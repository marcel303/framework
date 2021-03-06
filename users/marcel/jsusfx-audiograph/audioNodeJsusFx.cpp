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
#include "audioNodeJsusFx.h"
#include "audioResource.h"
#include "graphEdit.h"
#include "Log.h"

#include "jsusfx.h"
#include "jsusfx_file.h"

#include "framework.h"
#include "gfx-framework.h"
#include "jsusfx-framework.h"

#define SLIDER_INDEX(i) (i)
#define AUDIOINPUT_INDEX(i) (sliderInputs.size() + (i))

//

#include "jsusfx_serialize.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

struct AudioResource_JsusFx : AudioResourceBase
{
	JsusFxSerializationData serializationData;
	
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
	
	int version = 0;
	
	// we need to create a jsusfx instance to visually edit the resource
	JsusFxPathLibrary_Basic pathLibrary;
	JsusFxFileAPI_Basic fileAPI;
	JsusFx_Framework jsusFx;
	JsusFxGfx_Framework gfx;
	bool jsusFxIsValid;
	
	bool sliderIsActive[JsusFx::kMaxSliders] = { };
	
	ResourceEditor_JsusFx(const char * filename, const char * dataRoot, const char * searchPath)
		: GraphEdit_ResourceEditorBase(400, 400)
		, resource(nullptr)
		, pathLibrary(dataRoot)
		, fileAPI()
		, jsusFx(pathLibrary)
		, gfx(jsusFx)
		, jsusFxIsValid(false)
	{
		JsusFx::init();
		
		// setup interfaces
		
		pathLibrary.addSearchPath(searchPath);
		
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
	
	static void doSlider(
		const bool tick,
		const bool draw,
		JsusFx & fx,
		JsusFx_Slider & slider,
		int x, int y,
		bool & isActive)
	{
		const int sx = 200;
		const int sy = 12;
		
		if (tick)
		{
			const bool isInside =
				x >= 0 && x <= sx &&
				y >= 0 && y <= sy;
				
			if (isInside && mouse.wentDown(BUTTON_LEFT))
				isActive = true;
			
			if (mouse.wentUp(BUTTON_LEFT))
				isActive = false;
			
			if (isActive)
			{
				const float t = x / float(sx);
				const float v = slider.min + (slider.max - slider.min) * t;
				fx.moveSlider(&slider - fx.sliders, v);
			}
		}
		
		if (draw)
		{
			setColor(0, 0, 255, 127);
			const float t = (slider.getValue() - slider.min) / (slider.max - slider.min);
			drawRect(0, 0, sx * t, sy);
			
			if (slider.isEnum)
			{
				const int enumIndex = (int)slider.getValue();
				
				if (enumIndex >= 0 && enumIndex < slider.enumNames.size())
				{
					setColor(colorWhite);
					drawText(sx/2.f, sy/2.f, 10.f, 0.f, 0.f, "%s", slider.enumNames[enumIndex].c_str());
				}
			}
			else
			{
				setColor(colorWhite);
				drawText(sx/2.f, sy/2.f, 10.f, 0.f, 0.f, "%s", slider.desc);
			}
			
			setColor(63, 31, 255, 127);
			drawRectLine(0, 0, sx, sy);
		}
	}

	void doSliders(const bool tick, const bool draw, const bool inputIsCaptured)
	{
		int x = 10;
		int y = 10;
		
		int sliderIndex = 0;
		
		for (auto & slider : jsusFx.sliders)
		{
			if (slider.exists && slider.desc[0] != '-')
			{
				gxPushMatrix();
				gxTranslatef(x, y, 0);
				doSlider(
					tick, draw,
					jsusFx,
					slider,
					mouse.x - x,
					mouse.y - y,
					sliderIsActive[sliderIndex]);
				gxPopMatrix();
				
				y += 16;
			}
			
			sliderIndex++;
		}
	}
	
	virtual bool tick(const float dt, const bool inputIsCaptured) override
	{
		if (jsusFxIsValid)
		{
			gfx.setup(nullptr, sx, sy, mouse.x, mouse.y, inputIsCaptured == false);
			
			jsusFx.process(nullptr, nullptr, AUDIO_UPDATE_SIZE, 0, 0);
			
			doSliders(true, false, inputIsCaptured);
		}
		
		if (jsusFxIsValid && resource != nullptr)
		{
			if (resource->version != version)
			{
				resource->lock();
				{
					version = resource->version;
					
					// the resource changed without us knowing it. perhaps there's a second editor operating on it. refresh!
					
					JsusFxSerializer_Basic serializer(resource->serializationData);
					
					jsusFx.serialize(serializer, false);
				}
				resource->unlock();
			}
			
			// it's difficult to tell if the user interacted with the effect's ui and made changes. so we just
			// serialize the effect each tick and see if it's different from the current serialization data
			
			JsusFxSerializationData serializationData;
			
			JsusFxSerializer_Basic serializer(serializationData);
		
			if (jsusFx.serialize(serializer, true))
			{
				if (serializationData != resource->serializationData)
				{
					// it changed! update the resource
					
					resource->lock();
					{
						resource->serializationData = serializationData;
						
						resource->version++;
						
						version = resource->version;
					}
					resource->unlock();
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
			
			const_cast<ResourceEditor_JsusFx*>(this)->doSliders(false, true, false);
		}
	}
	
	virtual void setResource(const GraphNode & node, const char * type, const char * name) override
	{
		Assert(resource == nullptr);
		
		if (createAudioNodeResource(node, type, name, resource))
		{
			version = resource->version;
			
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
	std::string dataRoot;
	std::string searchPath;
	
	std::string filename;
	
	std::vector<AudioNodeJsusFx::SliderInput> sliderInputs;
	
	int numInputs = 0;
	int numOutputs = 0;
	
	void initFromJsusFx(const JsusFx_Framework & jsusFx, const char * _filename, const char * _typeName)
	{
		filename = _filename;
		
		typeName = String::FormatC("jsusfx.%s", _typeName);
		
		displayName = jsusFx.desc;
		
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
			
			const char * sliderDesc =
				slider.desc[0] == 0
				? slider.name
				: slider.desc[0] == '-'
				? slider.desc + 1
				: slider.desc;
			
			if (slider.isEnum)
			{
				// add enumeration type registration with a name unique to this node type ..
				
				const std::string enumName = String::FormatC("%s_%s", typeName.c_str(), slider.name);
				
				AudioEnumTypeRegistration * e = new AudioEnumTypeRegistration();
				e->enumName = enumName;
				for (int i = 0; i < slider.enumNames.size(); ++i)
					e->elem(slider.enumNames[i].c_str());
				
				// .. and add the enum input itself
				
				inEnum(sliderInput.name.c_str(), enumName.c_str(), (int)slider.def, sliderDesc);
			}
			else
			{
				char defaultString[64];
				sprintf_s(defaultString, sizeof(defaultString), "%g", slider.def);
				
				in(sliderInput.name.c_str(), "audioValue", defaultString, sliderDesc);
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
		
		mainResourceType = "jsusfx";
		mainResourceName = "editorData";
	}
};

static AudioNodeBase * createJsusFxNode(const AudioNodeTypeRegistration_JsusFx * r)
{
	AudioNodeJsusFx * fx = new AudioNodeJsusFx(r->dataRoot.c_str(), r->searchPath.c_str());
	
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

void createJsusFxAudioNodes(const char * dataRoot, const char * searchPath, const bool recurse)
{
	JsusFx::init();
	
	auto filenames = listFiles(searchPath, recurse);
	
	JsusFxPathLibrary_Basic pathLibrary(dataRoot);
	
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
		
		r->dataRoot = dataRoot;
		r->searchPath = searchPath;
		
		const char * typeName = filename.c_str();
		for (size_t i = 0; searchPath[i] != 0 && *typeName == searchPath[i]; ++i)
			typeName++;
		while (*typeName == '/')
			typeName++;
		
		r->initFromJsusFx(jsusFx, filename.c_str(), typeName);
		
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
			
			return new ResourceEditor_JsusFx(
				r->filename.c_str(),
				r->dataRoot.c_str(),
				r->searchPath.c_str());
		};
	}
}

//

AudioNodeJsusFx::AudioNodeJsusFx(const char * dataRoot, const char * searchPath)
	: AudioNodeBase()
	, filename()
	, numAudioInputs(0)
	, numAudioOutputs(0)
	, audioOutputs()
	, pathLibrary(nullptr)
	, jsusFx(nullptr)
	, jsusFxIsValid(false)
	, jsusFx_fileAPI(nullptr)
	, jsusFx_gfxAPI(nullptr)
	, resource(nullptr)
	, resourceVersion(-1)
{
	Assert(pathLibrary == nullptr);
	pathLibrary = new JsusFxPathLibrary_Basic(dataRoot);
	pathLibrary->addSearchPath(searchPath);
}

AudioNodeJsusFx::~AudioNodeJsusFx()
{
	free();
	
	freeAudioNodeResource(resource);
	Assert(resource == nullptr);
	
	delete pathLibrary;
	pathLibrary = nullptr;
}

void AudioNodeJsusFx::load(const char * filename)
{
	free();
	
	//
	
	Assert(jsusFx == nullptr);
	jsusFx = new JsusFx_Framework(*pathLibrary);
	
	Assert(jsusFx_fileAPI == nullptr);
	jsusFx_fileAPI = new JsusFxFileAPI_Basic();
	jsusFx_fileAPI->init(jsusFx->m_vm);
	jsusFx->fileAPI = jsusFx_fileAPI;
	
	Assert(jsusFx_gfxAPI == nullptr);
	jsusFx_gfxAPI = new JsusFxGfx();
	jsusFx_gfxAPI->init(jsusFx->m_vm);
	jsusFx->gfx = jsusFx_gfxAPI;
	
	//
	
	Assert(jsusFxIsValid == false);
	
	jsusFxIsValid = jsusFx->compile(*pathLibrary, filename, JsusFx::kCompileFlag_CompileSerializeSection);
	
	if (jsusFxIsValid)
	{
		updateFromResource();
		
		jsusFx->prepare(SAMPLE_RATE, AUDIO_UPDATE_SIZE);
	}
}

void AudioNodeJsusFx::free()
{
	jsusFxIsValid = false;
	
	delete jsusFx_gfxAPI;
	jsusFx_gfxAPI = nullptr;
	
	if (jsusFx != nullptr)
		jsusFx->gfx = nullptr;
		
	delete jsusFx_fileAPI;
	jsusFx_fileAPI = nullptr;
	
	if (jsusFx != nullptr)
		jsusFx->fileAPI = nullptr;
	
	delete jsusFx;
	jsusFx = nullptr;
	
	resourceVersion = -1;
}

void AudioNodeJsusFx::clearOutputs()
{
	for (auto & audioOutput : audioOutputs)
		audioOutput.setZero();
}

void AudioNodeJsusFx::updateFromResource()
{
	if (resourceVersion != resource->version)
	{
		resource->lock();
		{
			resourceVersion = resource->version;
			
			if (jsusFxIsValid)
			{
				JsusFxSerializer_Basic serializer(resource->serializationData);
				
				jsusFx->serialize(serializer, false);
				
				// update slider values from resource
				
				for (auto & sliderInput : sliderInputs)
				{
					Assert(jsusFx->sliders[sliderInput.sliderIndex].exists);
					
					// note : slider input default comes from either the serialized
					// resource data (when set), or from the original default value
					// set for the JSFX slider, if the aforementioned resource data
					// isn't set. this ensures that when for instance a knob inside
					// the resource editor is turned, the knob value will be used
					// (when no connection exists to the input, which would override
					// the value)
					
					// find the value inside the serialized resource data (if it exists)
					
					sliderInput.valueFromResource = 0.f;
					sliderInput.hasValueFromResource = false;
					
					for (auto & resourceSlider : resource->serializationData.sliders)
					{
						if (resourceSlider.index == sliderInput.sliderIndex)
						{
							Assert(sliderInput.hasValueFromResource == false);
							sliderInput.valueFromResource = resourceSlider.value;
							sliderInput.hasValueFromResource = true;
						}
					}
				}
			}
		}
		resource->unlock();
	}
}

void AudioNodeJsusFx::initSelf(const GraphNode & node)
{
	Assert(resource == nullptr);
	createAudioNodeResource(node, "jsusfx", "editorData", resource);
	
	load(filename.c_str());
	
	if (jsusFxIsValid)
	{
		updateFromResource();
		
		jsusFx->prepare(SAMPLE_RATE, AUDIO_UPDATE_SIZE);
	}
}

void AudioNodeJsusFx::tick(const float dt)
{
	if (isPassthrough)
	{
		for (int i = 0; i < audioOutputs.size() && i < numAudioInputs; ++i)
		{
			const AudioFloat * audioInput = getInputAudioFloat(AUDIOINPUT_INDEX(i), &AudioFloat::Zero);
			
			audioOutputs[i].set(*audioInput);
		}
		
		for (int i = numAudioInputs; i < audioOutputs.size(); ++i)
			audioOutputs[i].setZero();
		
		return;
	}
	
	if (jsusFxIsValid == false)
	{
		clearOutputs();
	}
	else
	{
		// update from resource, in case the resource is being edited in real-time
		
		updateFromResource();
	
		// update slider values
		
		AudioFloat defaultValue;
		
		for (auto & sliderInput : sliderInputs)
		{
			Assert(jsusFx->sliders[sliderInput.sliderIndex].exists);
			
			// note : slider input default comes from either the serialized
			// resource data (when set), or from the original default value
			// set for the JSFX slider, if the aforementioned resource data
			// isn't set. this ensures that when for instance a knob inside
			// the resource editor is turned, the knob value will be used
			// (when no connection exists to the input, which would override
			// the value)
			
			// if we have a slider value from the resource, use it as the
			// default value for fetching the input value. otherwise, use
			// the default value for the socket as authored inside the JSFX file
			
			if (sliderInput.hasValueFromResource)
				defaultValue.setScalar(sliderInput.valueFromResource);
			else
				defaultValue.setScalar(sliderInput.defaultValue);
			
			// fetch the value, and move the slider to the new value
			// note : moveSlider will clamp the value to the value
			// range for the slider
			
			const float value = getInputAudioFloat(sliderInput.socketIndex, &defaultValue)->getMean();
			
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
	if (jsusFx == nullptr)
		return;
		
	for (int i = 0; i < JsusFx::kMaxSliders; ++i)
	{
		auto & slider = jsusFx->sliders[i];
		
		if (!slider.exists)
			continue;
		
		d.add("slider %s: default=%f, increment=%f, desc=%s",
			slider.name,
			slider.def,
			slider.inc,
			slider.desc);
	}
	
#if false
	for (auto & sliderInput : sliderInputs)
	{
		d.add("registered slider input %s. socket_index=%d, slider_index=%d",
			sliderInput.name.c_str(),
			sliderInput.socketIndex,
			sliderInput.sliderIndex);
	}
#endif
}

#undef SLIDER_INDEX
#undef AUDIOINPUT_INDEX

void linkAudioNodes_JsusFx()
{
}
