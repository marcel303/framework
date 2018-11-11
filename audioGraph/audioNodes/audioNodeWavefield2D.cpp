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

#include "audioNodeWavefield2D.h"
#include "Noise.h"
#include "wavefield.h"
#include <cmath>


#include "audioResource.h"
#include "StringEx.h"
#include "tinyxml2.h"
#include "tinyxml2_helpers.h"

struct AudioResource_Wavefield2D : AudioResourceBase
{
	float f[Wavefield2D::kMaxElems][Wavefield2D::kMaxElems];
	int numElems;
	int version;
	
	AudioResource_Wavefield2D()
		: numElems(0)
		, version(0)
	{
	}
	
	virtual void save(tinyxml2::XMLPrinter * printer) override
	{
		printer->PushAttribute("numElems", numElems);
		
		for (int i = 0; i < numElems; ++i)
		{
			char name[16];
			sprintf_s(name, sizeof(name), "f_%d", i);
			
			pushAttrib_array(printer, name, f[i], sizeof(f[i][0]), numElems);
		}
	}
	
	virtual void load(tinyxml2::XMLElement * elem) override
	{
		numElems = intAttrib(elem, "numElems", 0);
		numElems = clamp(numElems, 0, Wavefield2D::kMaxElems);
		
		for (int i = 0; i < numElems; ++i)
		{
			char name[16];
			sprintf_s(name, sizeof(name), "f_%d", i);
			
			arrayAttrib(elem, name, f[i], sizeof(f[i][0]), numElems);
		}
		
		version++;
	}
};

AUDIO_RESOURCE_TYPE(AudioResource_Wavefield2D, "wavefield.2d");

//

#include "graph.h"
#include "Noise.h"
#include "ui.h"

struct ResourceEditor_Wavefield2D : GraphEdit_ResourceEditorBase
{
	AudioResource_Wavefield2D * resource;
	
	UiState uiState;
	
	Wavefield2D wavefield;
	
	AudioRNG rng;
	
	ResourceEditor_Wavefield2D()
		: GraphEdit_ResourceEditorBase(400, 400)
		, resource(nullptr)
		, uiState()
		, wavefield()
		, rng()
	{
		uiState.sx = sx;
		uiState.textBoxTextOffset = 40;
	}
	
	virtual ~ResourceEditor_Wavefield2D() override
	{
		freeAudioNodeResource(resource);
		Assert(resource == nullptr);
	}
	
	void randomize()
	{
		const double xRatio = rng.nextd(0.0, 1.0 / 10.0);
		const double yRatio = rng.nextd(0.0, 1.0 / 10.0);
		const double randomFactor = rng.nextd(0.0, 1.0);
		//const double cosFactor = rng.nextd(0.0, 1.0);
		const double cosFactor = 0.0;
		const double perlinFactor = rng.nextd(0.0, 1.0);
		
		for (int x = 0; x < resource->numElems; ++x)
		{
			for (int y = 0; y < resource->numElems; ++y)
			{
				double f = 1.0;
			
				f *= Wavefield::lerp<double>(1.0, rng.nextd(0.f, 1.f), randomFactor);
				f *= Wavefield::lerp<double>(1.0, (std::cos(x * xRatio + y * yRatio) + 1.0) / 2.0, cosFactor);
				//f = 1.0 - std::pow(f[x][y], 2.0);
			
				//f = 1.0 - std::pow(rng.nextd(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
				f *= Wavefield::lerp<double>(1.0, scaled_octave_noise_2d(16, .4f, 1.f / 20.f, 0.f, 1.f, x, y), perlinFactor);
				
				resource->f[x][y] = f;
			}
		}
	}
	
	void doWavefield(const float dt)
	{
		if (resource == nullptr)
			return;
		
		if (g_doActions)
		{
			for (int x = 0; x < resource->numElems; ++x)
				for (int y = 0; y < resource->numElems; ++y)
					wavefield.f[x][y] = resource->f[x][y];
			
			wavefield.numElems = resource->numElems;
			
			for (int i = 0; i < 10; ++i)
				wavefield.tick(dt / 100.0, 100000.0, 0.8, 0.8, true);
		}
		
		if (g_doDraw)
		{
			gxPushMatrix();
			gxTranslatef(x, y, 0);
			gxTranslatef(sx/2, sy/2, 0);
			gxScalef(sx / float(resource->numElems), sy / float(resource->numElems), 1.f);
			gxTranslatef(-(resource->numElems - 1.f)/2.f, -(resource->numElems - 1.f)/2.f, 0.f);
			
			hqBegin(HQ_FILLED_CIRCLES);
			{
				for (int x = 0; x < wavefield.numElems; ++x)
				{
					for (int y = 0; y < wavefield.numElems; ++y)
					{
						const float p = wavefield.sample(x, y);
						const float a = saturate(wavefield.f[x][y]);
						
						setColorf(1.f, 1.f, 1.f, a);
						hqFillCircle(x, y, .4f + std::abs(p) * 3.f);
					}
				}
			}
			hqEnd();
			
			gxPopMatrix();
		}
	}
	
	void doMenus(const bool doAction, const bool doDraw, const float dt)
	{
		uiState.sx = sx;
		uiState.x = x;
		uiState.y = y;
		
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
					const int y = rand() % wavefield.numElems;
					
					wavefield.doGaussianImpact(x, y, 1, 1.f);
				}
			}
			x += sx;
			
			if (resource != nullptr)
			{
				int numElems = resource->numElems;
				
				doTextBox(numElems, "size", x, sx, false, dt);
				x += sx;
				
				numElems = Wavefield2D::roundNumElems(numElems);
				
				if (numElems != resource->numElems)
				{
					for (int x = resource->numElems; x < numElems; ++x)
						for (int y = 0; y < numElems; ++y)
							resource->f[x][y] = 1.f;
					for (int y = resource->numElems; y < numElems; ++y)
						for (int x = 0; x < numElems; ++x)
							resource->f[x][y] = 1.f;
					
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
		
		const_cast<ResourceEditor_Wavefield2D*>(this)->doMenus(false, true, 0.f);
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

AUDIO_NODE_TYPE(AudioNodeWavefield2D)
{
	typeName = "wavefield.2d";
	
	in("size", "int", "16");
	in("gain", "audioValue", "1");
	in("pos.dampen", "audioValue");
	in("vel.dampen", "audioValue");
	in("tension", "audioValue", "1");
	in("wrap", "bool", "0");
	in("sample.pos.x", "audioValue", "0.5");
	in("sample.pos.y", "audioValue", "0.5");
	in("trigger!", "trigger");
	in("trigger.pos.x", "audioValue", "0.5");
	in("trigger.pos.y", "audioValue", "0.5");
	in("trigger.amount", "audioValue", "0.5");
	in("trigger.size", "audioValue", "1");
	in("randomize!", "trigger");
	out("audio", "audioValue");
	
	resourceTypeName = "wavefield.2d";
	
	createResourceEditor = [](void * data) -> GraphEdit_ResourceEditorBase*
	{
		return new ResourceEditor_Wavefield2D();
	};
}

AudioNodeWavefield2D::AudioNodeWavefield2D()
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
	addInput(kInput_SampleLocationX, kAudioPlugType_FloatVec);
	addInput(kInput_SampleLocationY, kAudioPlugType_FloatVec);
	addInput(kInput_Trigger, kAudioPlugType_Trigger);
	addInput(kInput_TriggerLocationX, kAudioPlugType_FloatVec);
	addInput(kInput_TriggerLocationY, kAudioPlugType_FloatVec);
	addInput(kInput_TriggerAmount, kAudioPlugType_FloatVec);
	addInput(kInput_TriggerSize, kAudioPlugType_FloatVec);
	addInput(kInput_Randomize, kAudioPlugType_Trigger);
	addOutput(kOutput_Audio, kAudioPlugType_FloatVec, &audioOutput);

	wavefield = new Wavefield2D();
}

AudioNodeWavefield2D::~AudioNodeWavefield2D()
{
	delete wavefield;
	wavefield = nullptr;
	
	freeAudioNodeResource(wavefieldData);
}

void AudioNodeWavefield2D::randomize()
{
	const double xRatio = rng.nextd(0.0, 1.0 / 10.0);
	const double yRatio = rng.nextd(0.0, 1.0 / 10.0);
	const double randomFactor = rng.nextd(0.0, 1.0);
	//const double cosFactor = rng.nextd(0.0, 1.0);
	const double cosFactor = 0.0;
	const double perlinFactor = rng.nextd(0.0, 1.0);
	
	for (int x = 0; x < wavefield->numElems; ++x)
	{
		for (int y = 0; y < wavefield->numElems; ++y)
		{
			wavefield->p[x][y] = 0.0;
			wavefield->v[x][y] = 0.0;
			wavefield->d[x][y] = 0.0;
			
			wavefield->f[x][y] = 1.0;
			
			wavefield->f[x][y] *= Wavefield::lerp<double>(1.0, rng.nextd(0.f, 1.f), randomFactor);
			wavefield->f[x][y] *= Wavefield::lerp<double>(1.0, (std::cos(x * xRatio + y * yRatio) + 1.0) / 2.0, cosFactor);
			//wavefield->f[x][y] = 1.0 - std::pow(wavefield->f[x][y], 2.0);
			
			//wavefield->f[x][y] = 1.0 - std::pow(rng.nextd(0.f, 1.f), 2.0) * (std::cos(x / 4.32) + 1.0)/2.0 * (std::cos(y / 3.21) + 1.0)/2.0;
			wavefield->f[x][y] *= Wavefield::lerp<double>(1.0, scaled_octave_noise_2d(16, .4f, 1.f / 20.f, 0.f, 1.f, x, y), perlinFactor);
		}
	}
}

void AudioNodeWavefield2D::init(const GraphNode & node)
{
	createAudioNodeResource(node, "wavefield.2d", "editorData", wavefieldData);
}

void AudioNodeWavefield2D::tick(const float _dt)
{
	audioCpuTimingBlock(AudioNodeWavefield2D);
	
	const AudioFloat defaultTension(1.f);
	
	const AudioFloat * gain = getInputAudioFloat(kInput_Gain, &AudioFloat::One);
	const AudioFloat * positionDampening = getInputAudioFloat(kInput_PositionDampening, &AudioFloat::Zero);
	const AudioFloat * velocityDampening = getInputAudioFloat(kInput_VelocityDampening, &AudioFloat::Zero);
	const AudioFloat * tension = getInputAudioFloat(kInput_Tension, &defaultTension);
	const bool wrap = getInputBool(kInput_Wrap, false);
	const AudioFloat * sampleLocationX = getInputAudioFloat(kInput_SampleLocationX, &AudioFloat::Half);
	const AudioFloat * sampleLocationY = getInputAudioFloat(kInput_SampleLocationY, &AudioFloat::Half);
// todo : remove size input
	const int size = getInputInt(kInput_Size, 16);
	
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
		
		for (int x = 0; x < wavefield->numElems; ++x)
			for (int y = 0; y < wavefield->numElems; ++y)
				wavefield->f[x][y] = wavefieldData->f[x][y];
		
		currentDataVersion = wavefieldData->version;
	}
	
	//
	
	audioOutput.setVector();
	
	positionDampening->expand();
	velocityDampening->expand();
	tension->expand();
	sampleLocationX->expand();
	sampleLocationY->expand();
	
	//
	
	const double dt = 1.0 / double(SAMPLE_RATE);
	
	const double maxTension = 2000000000.0;
	
	for (int i = 0; i < AUDIO_UPDATE_SIZE; ++i)
	{
		const double c = Wavefield::clamp<double>(tension->samples[i] * 1000000.0, -maxTension, +maxTension);
		
		wavefield->tick(dt, c, 1.0 - velocityDampening->samples[i], 1.0 - positionDampening->samples[i], wrap == false);
		
		audioOutput.samples[i] = wavefield->sample(
			sampleLocationX->samples[i] * wavefield->numElems,
			sampleLocationY->samples[i] * wavefield->numElems);
	}
	
	audioOutput.mul(*gain);
}

void AudioNodeWavefield2D::handleTrigger(const int inputSocketIndex)
{
	if (inputSocketIndex == kInput_Trigger)
	{
		const float triggerPositionX = getInputAudioFloat(kInput_TriggerLocationX, &AudioFloat::Half)->getMean();
		const float triggerPositionY = getInputAudioFloat(kInput_TriggerLocationY, &AudioFloat::Half)->getMean();
		const float triggerAmount = getInputAudioFloat(kInput_TriggerAmount, &AudioFloat::Half)->getMean();
		const float triggerSize = getInputAudioFloat(kInput_TriggerSize, &AudioFloat::One)->getMean();
		
		if (wavefield->numElems > 0)
		{
			const int elemX = int(std::round(std::abs(triggerPositionX) * wavefield->numElems)) % wavefield->numElems;
			const int elemY = int(std::round(std::abs(triggerPositionY) * wavefield->numElems)) % wavefield->numElems;
			
			if (triggerSize == 1.f)
				wavefield->d[elemX][elemY] += triggerAmount;
			else
				wavefield->doGaussianImpact(elemX, elemX, triggerSize, triggerAmount);
		}
	}
	else if (inputSocketIndex == kInput_Randomize)
	{
		randomize();
	}
}

