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

#include "audioNodeWavefield1D.h"
#include "Noise.h"
#include "wavefield.h"
#include <cmath>

#include "audioResource.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

struct AudioResource_Wavefield1D : AudioResourceBase
{
	double f[Wavefield1D::kMaxElems];
	int numElems;
	int version;
	
	AudioResource_Wavefield1D()
		: numElems(0)
		, version(0)
	{
	}
	
	virtual void save(tinyxml2::XMLPrinter * printer) override
	{
		printer->PushAttribute("numElems", numElems);
		
		pushAttrib_array(printer, "f", f, sizeof(f[0]), numElems);
	}
	
	virtual void load(tinyxml2::XMLElement * elem) override
	{
		numElems = intAttrib(elem, "numElems", 0);
		numElems = Wavefield::clamp(numElems, 0, Wavefield1D::kMaxElems);
		
		arrayAttrib(elem, "f", f, sizeof(f[0]), numElems);
		
		version++;
	}
};

AUDIO_RESOURCE_TYPE(AudioResource_Wavefield1D, "wavefield.1d");

//

#include "framework.h"
#include "graph.h"
#include "Noise.h"
#include "ui.h"

struct ResourceEditor_Wavefield1D : GraphEdit_ResourceEditorBase
{
	AudioResource_Wavefield1D * resource;
	
	UiState uiState;
	
	Wavefield1D wavefield;
	
	ResourceEditor_Wavefield1D()
		: GraphEdit_ResourceEditorBase(700, 256)
		, resource(nullptr)
		, uiState()
		, wavefield()
	{
		uiState.sx = sx;
		uiState.textBoxTextOffset = 40;
	}
	
	virtual ~ResourceEditor_Wavefield1D() override
	{
		freeAudioNodeResource(resource);
		Assert(resource == nullptr);
	}
	
	virtual void afterPositionChanged() override
	{
		uiState.x = x;
		uiState.y = y;
	}
	
	virtual void afterSizeChanged() override
	{
		uiState.sx = sx;
	}
	
	void randomize()
	{
		const double xRatio = random(0.0, 1.0 / 10.0);
		const double randomFactor = random(0.0, 1.0);
		//const double cosFactor = random(0.0, 1.0);
		const double cosFactor = 0.0;
		const double perlinFactor = random(0.0, 1.0);
		
		for (int x = 0; x < resource->numElems; ++x)
		{
			resource->f[x] = 1.0;
			
			resource->f[x] *= lerp(1.0, random(0.0, 1.0), randomFactor);
			resource->f[x] *= lerp(1.0, (std::cos(x * xRatio) + 1.0) / 2.0, cosFactor);
			//resource->f[x] = 1.0 - std::pow(m_wavefield.f[x], 2.0);
			
			//resource->f[x] = 1.0 - std::pow(random(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
			resource->f[x] *= lerp(1.0, (double)scaled_octave_noise_1d(16, .4f, 1.f / 20.f, 0.f, 1.f, x), perlinFactor);
		}
	}
	
	void doWavefield(const float dt)
	{
		if (resource == nullptr)
			return;
		
		if (g_doActions)
		{
			for (int i = 0; i < resource->numElems; ++i)
				wavefield.f[i] = resource->f[i];
			wavefield.numElems = resource->numElems;
			
			for (int i = 0; i < 10; ++i)
				wavefield.tick(dt / 10.0, 10000.0, 0.8, 0.8, true);
		}
		
		if (g_doDraw)
		{
			gxPushMatrix();
			gxTranslatef(x, y, 0);
			gxTranslatef(sx/2, sy/2, 0);
			gxScalef(sx / float(resource->numElems), sy/2, 1.f);
			gxTranslatef(-(resource->numElems - 1.f)/2.f, 0.f, 0.f);
			
			setColor(colorWhite);
			
			const float r = clamp(sx / float(resource->numElems + 1.f) / 2.f, 1.f, 6.f);
			
			hqBegin(HQ_FILLED_CIRCLES, true);
			{
				const double * p = wavefield.p;
				const double * f = resource->f;
				
				for (int i = 0; i < resource->numElems; ++i)
				{
					const float h = p[i];
					//const float h = 0.f;
					const float a = f[i] / 2.f;
					
					setLumif(a);
					hqFillCircle(i, h, r);
				}
			}
			hqEnd();
			
		#if 1
			hqBegin(HQ_LINES, true);
			{
				const double * p = wavefield.p;
				const double * f = resource->f;
				
				for (int i = 0; i < resource->numElems; ++i)
				{
					const float h = p[i];
					const float a = f[i] / 2.f;
					
					setLumif(a);
					hqLine(i, 0.f, r, i, h, r);
				}
			}
			hqEnd();
		#endif

		#if 0
			if (enabled)
			{
				hqBegin(HQ_FILLED_CIRCLES);
				{
					const float p = wavefield.sample(sampleLocation);
					const float a = 1.f;
					
					setColorf(1.f, 1.f, 0.f, a);
					hqFillCircle(sampleLocation, p, 1.f);
				}
				hqEnd();
			}
		#endif
			
			hqBegin(HQ_LINES, true);
			{
				setColor(colorGreen);
				hqLine(- .5f, -1.f, 1.f, resource->numElems- .5f, -1.f, 1.f);
				hqLine(- .5f, +1.f, 1.f, resource->numElems- .5f, +1.f, 1.f);
			}
			hqEnd();
			
			gxPopMatrix();
		}
	}
	
	void doMenus(const bool doAction, const bool doDraw, const float dt)
	{
		makeActive(&uiState, doAction, doDraw);
		pushMenu("buttons");
		{
			doWavefield(dt);
			
			float x = 0.f;
			float sx = 1.f / 6;
			
			if (doButton("randomize", x, sx, false))
			{
				randomize();
				
				resource->version++;
			}
			x += sx;
			
			if (doButton("play", x, sx, false))
			{
				if (wavefield.numElems > 0)
				{
					const int x = rand() % wavefield.numElems;
					
					wavefield.doGaussianImpact(x, 1, 1.f);
				}
			}
			x += sx;
			
			if (resource != nullptr)
			{
				int numElems = resource->numElems;
				
				doTextBox(numElems, "size", x, sx, false, dt);
				x += sx;
				
				numElems = Wavefield1D::roundNumElems(numElems);
				
				if (numElems != resource->numElems)
				{
					for (int i = resource->numElems; i < numElems; ++i)
						resource->f[i] = 1.f;
					
					resource->numElems = numElems;
					
					resource->version++;
					
					//
					
					wavefield.init(numElems);
				}
			}
			
			doBreak();
		}
		popMenu();
	}
	
	virtual bool tick(const float dt, const bool inputIsCaptured) override
	{
		doMenus(true, false, dt);
		
		return uiState.activeElem != nullptr;
	}
	
	virtual void draw() const override
	{
		gxPushMatrix();
		{
			gxTranslatef(x, y, 0);
			
			setColor(colorBlack);
			hqBegin(HQ_FILLED_ROUNDED_RECTS);
			{
				hqFillRoundedRect(0, 0, sx, sy, 12.f);
			}
			hqEnd();
		}
		gxPopMatrix();
		
		const_cast<ResourceEditor_Wavefield1D*>(this)->doMenus(false, true, 0.f);
	}
	
	virtual void setResource(const GraphNode & node, const char * type, const char * name) override
	{
		Assert(resource == nullptr);
		
		if (createAudioNodeResource(node, type, name, resource))
		{
			wavefield.init(resource->numElems);
		}
	}
	
	virtual bool serializeResource(std::string & text) const override
	{
		if (resource != nullptr)
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
		else
		{
			return false;
		}
	}
};

//

AUDIO_NODE_TYPE(wavefield_1d, AudioNodeWavefield1D)
{
	typeName = "wavefield.1d";
	
	resourceTypeName = "wavefield.1d";
	
	createResourceEditor = []() -> GraphEdit_ResourceEditorBase*
	{
		return new ResourceEditor_Wavefield1D();
	};
	
	in("size", "int", "16");
	in("gain", "audioValue", "1");
	in("pos.dampen", "audioValue");
	in("vel.dampen", "audioValue");
	in("tension", "audioValue", "1");
	in("wrap", "bool", "0");
	in("sample.pos", "audioValue", "0.5");
	in("trigger!", "trigger");
	in("trigger.pos", "audioValue", "0.5");
	in("trigger.amount", "audioValue", "0.5");
	in("trigger.size", "1");
	in("randomize!", "trigger");
	out("audio", "audioValue");
}

AudioNodeWavefield1D::AudioNodeWavefield1D()
	: AudioNodeBase()
	, wavefieldData(nullptr)
	, currentDataVersion(-1)
	, wavefield(nullptr)
	, rng()
	, audioOutput()
{
	resizeSockets(kInput_COUNT, kOutput_COUNT);
	addInput(kInput_Size, kAudioPlugType_Int);
	addInput(kInput_Gain, kAudioPlugType_FloatVec);
	addInput(kInput_PositionDampening, kAudioPlugType_FloatVec);
	addInput(kInput_VelocityDampening, kAudioPlugType_FloatVec);
	addInput(kInput_Tension, kAudioPlugType_FloatVec);
	addInput(kInput_Wrap, kAudioPlugType_Bool);
	addInput(kInput_SampleLocation, kAudioPlugType_FloatVec);
	addInput(kInput_Trigger, kAudioPlugType_Trigger);
	addInput(kInput_TriggerLocation, kAudioPlugType_FloatVec);
	addInput(kInput_TriggerAmount, kAudioPlugType_FloatVec);
	addInput(kInput_TriggerSize, kAudioPlugType_FloatVec);
	addInput(kInput_Randomize, kAudioPlugType_Trigger);
	addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);

	wavefield = new Wavefield1D();
}

AudioNodeWavefield1D::~AudioNodeWavefield1D()
{
	delete wavefield;
	wavefield = nullptr;
	
	freeAudioNodeResource(wavefieldData);
}

void AudioNodeWavefield1D::randomize()
{
	const double xRatio = rng.nextd(0.0, 1.0 / 10.0);
	const double randomFactor = rng.nextd(0.0, 1.0);
	//const double cosFactor = rng.nextd(0.0, 1.0);
	const double cosFactor = 0.0;
	const double perlinFactor = rng.nextd(0.0, 1.0);
	
	for (int x = 0; x < wavefield->numElems; ++x)
	{
		wavefield->p[x] = 0.0;
		wavefield->v[x] = 0.0;
		wavefield->d[x] = 0.0;
		
		wavefield->f[x] = 1.0;
		wavefield->f[x] *= Wavefield::lerp<double>(1.0, rng.nextd(0.0, 1.0), randomFactor);
		wavefield->f[x] *= Wavefield::lerp<double>(1.0, (std::cos(x * xRatio) + 1.0) / 2.0, cosFactor);
		
		//wavefield->f[x] = 1.0 - std::pow(m_wavefield.f[x], 2.0);
		//wavefield->f[x] = 1.0 - std::pow(rng.nextd(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
		
		wavefield->f[x] *= Wavefield::lerp<double>(1.0, scaled_octave_noise_1d(16, .4f, 1.f / 20.f, 0.f, 1.f, x), perlinFactor);
	}
}

void AudioNodeWavefield1D::init(const GraphNode & node)
{
	createAudioNodeResource(node, "wavefield.1d", "editorData", wavefieldData);
}

void AudioNodeWavefield1D::tick(const float _dt)
{
	audioCpuTimingBlock(AudioNodeWavefield1D);
	
	const AudioFloat defaultTension(1.f);
	
	const AudioFloat * gain = getInputAudioFloat(kInput_Gain, &AudioFloat::One);
	const AudioFloat * positionDampening = getInputAudioFloat(kInput_PositionDampening, &AudioFloat::Zero);
	const AudioFloat * velocityDampening = getInputAudioFloat(kInput_VelocityDampening, &AudioFloat::Zero);
	const AudioFloat * tension = getInputAudioFloat(kInput_Tension, &defaultTension);
	const bool wrap = getInputBool(kInput_Wrap, false);
	const AudioFloat * sampleLocation = getInputAudioFloat(kInput_SampleLocation, &AudioFloat::Half);
	//const int size = getInputInt(kInput_Size, 16);
	
	//

	if (isPassthrough)
	{
		audioOutput.setScalar(0.f);
		return;
	}
	
	//
	
	if (wavefieldData->version != currentDataVersion)
	{
		wavefield->init(wavefieldData->numElems);
		
		for (int i = 0; i < wavefield->numElems; ++i)
			wavefield->f[i] = wavefieldData->f[i];
		
		currentDataVersion = wavefieldData->version;
	}
	
	//
	
	audioOutput.setVector();
	
	positionDampening->expand();
	velocityDampening->expand();
	tension->expand();
	sampleLocation->expand();
	
	//
	
	const double dt = 1.0 / double(SAMPLE_RATE);
	
	const double maxTension = 2000000000.0;
	
	for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
	{
		const double c = Wavefield::clamp<double>(tension->samples[i] * 1000000.0, -maxTension, +maxTension);
		
		wavefield->tick(dt, c, 1.0 - velocityDampening->samples[i], 1.0 - positionDampening->samples[i], wrap == false);
		
		audioOutput.samples[i] = wavefield->sample(sampleLocation->samples[i] * wavefield->numElems);
	}
	
	audioOutput.mul(*gain);
}

void AudioNodeWavefield1D::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_Trigger)
	{
		const float triggerPosition = getInputAudioFloat(kInput_TriggerLocation, &AudioFloat::Half)->getMean();
		const float triggerAmount = getInputAudioFloat(kInput_TriggerAmount, &AudioFloat::Half)->getMean();
		const float triggerSize = getInputAudioFloat(kInput_TriggerSize, &AudioFloat::One)->getMean();
		
		if (wavefield->numElems > 0)
		{
			const int elemIndex = int(std::round(triggerPosition * wavefield->numElems)) % wavefield->numElems;
			
			if (triggerSize == 1.f)
				wavefield->d[elemIndex] += triggerAmount;
			else
				wavefield->d[elemIndex] += triggerAmount; // todo : implement gaussian impact
		}
	}
	else if (inputSocketIndex == kInput_Randomize)
	{
		randomize();
	}
}
