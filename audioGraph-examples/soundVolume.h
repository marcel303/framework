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

#pragma once

#include "Mat4x4.h"
#include "objects/binauralizer.h"
#include "soundmix.h"
#include "Vec3.h"

#define MAX_SAMPLELOCATIONS_PER_VOLUME 10 // center + nearest + eight vertices

struct MultiChannelAudioSource
{
	virtual void generate(const int channelIndex, SAMPLE_ALIGN16 float * __restrict audioBuffer, const int numSamples) = 0;
};

struct MultiChannelAudioSource_SoundVolume : MultiChannelAudioSource
{
	struct SampleLocation
	{
		float elevation;
		float azimuth;
		float gain;
		
		SampleLocation()
			: elevation(0.f)
			, azimuth(0.f)
			, gain(0.f)
		{
		}
	};
	
	binaural::Mutex * mutex;
	
	binaural::Binauralizer binauralizer;
	SampleLocation sampleLocation[MAX_SAMPLELOCATIONS_PER_VOLUME];
	
	AudioSource * source;
	
	ALIGN16 float samplesL[AUDIO_UPDATE_SIZE];
	ALIGN16 float samplesR[AUDIO_UPDATE_SIZE];
	
	MultiChannelAudioSource_SoundVolume()
		: mutex(nullptr)
		, binauralizer()
		, source(nullptr)
	{
	}
	
	void init(const binaural::HRIRSampleSet * sampleSet, binaural::Mutex * _mutex)
	{
		mutex = _mutex;
		
		binauralizer.init(sampleSet, _mutex);
	}
	
	void fillBuffers(const int numSamples)
	{
		// generate source audio
		
		memset(samplesL, 0, sizeof(samplesL));
		memset(samplesR, 0, sizeof(samplesR));
	
		ALIGN16 float sourceBuffer[AUDIO_UPDATE_SIZE];
		source->generate(sourceBuffer, numSamples);
		
		// fetch all of the HRIR sample datas we need based on the current elevation and azimuth pairs
		
		const binaural::HRIRSampleData * samples[3 * MAX_SAMPLELOCATIONS_PER_VOLUME];
		float sampleWeights[3 * MAX_SAMPLELOCATIONS_PER_VOLUME];
		float gain[MAX_SAMPLELOCATIONS_PER_VOLUME];
		
		mutex->lock();
		{
			for (int i = 0; i < MAX_SAMPLELOCATIONS_PER_VOLUME; ++i)
			{
				binauralizer.sampleSet->lookup_3(
					sampleLocation[i].elevation,
					sampleLocation[i].azimuth,
					samples + i * 3,
					sampleWeights + i * 3);
				
				gain[i] = sampleLocation[i].gain;
			}
		}
		mutex->unlock();
		
		// combine individual HRIRs into one combined HRIR
		
		binaural::HRIRSampleData combinedHrir;
		memset(&combinedHrir, 0, sizeof(combinedHrir));
		
		const binaural::HRIRSampleData ** samplesPtr = samples;
		float * sampleWeightsPtr = sampleWeights;
		
		for (int i = 0; i < MAX_SAMPLELOCATIONS_PER_VOLUME; ++i)
		{
			for (int j = 0; j < 3; ++j)
			{
				audioBufferAdd(combinedHrir.lSamples, samplesPtr[j]->lSamples, binaural::HRIR_BUFFER_SIZE, sampleWeightsPtr[j] * gain[i]);
				audioBufferAdd(combinedHrir.rSamples, samplesPtr[j]->rSamples, binaural::HRIR_BUFFER_SIZE, sampleWeightsPtr[j] * gain[i]);
			}
			
			samplesPtr += 3;
			sampleWeightsPtr += 3;
		}
		
		// run the binauralizer over the source audio
		
		binauralizer.provide(sourceBuffer, numSamples);
		
		binauralizer.generateLR(samplesL, samplesR, numSamples, &combinedHrir);
	}
	
	virtual void generate(const int channelIndex, SAMPLE_ALIGN16 float * __restrict audioBuffer, const int numSamples) override
	{
		Assert(channelIndex < 2);
		Assert(numSamples <= AUDIO_UPDATE_SIZE);
		
		if (source == nullptr)
		{
			memset(audioBuffer, 0, numSamples * sizeof(float));
			return;
		}
		
		//
		
		if (channelIndex == 0)
		{
			fillBuffers(numSamples);
		}
		
		//
		
		if (channelIndex == 0)
		{
			audioBufferAdd(audioBuffer, samplesL, numSamples);
		}
		else
		{
			audioBufferAdd(audioBuffer, samplesR, numSamples);
		}
	}
};

struct SoundVolume
{
	Mat4x4 transform;
	
	SoundVolume()
		: transform(true)
	{
	}
	
	Vec3 projectToSound(Vec3Arg v) const
	{
		return transform.CalcInv().Mul4(v);
	}
	
	Vec3 projectToWorld(Vec3Arg v) const
	{
		return transform.Mul4(v);
	}
	
	Vec3 nearestPointWorld(Vec3Arg targetWorld) const
	{
		const Vec3 target = projectToSound(targetWorld);
		
		const float x = std::max(-1.f, std::min(+1.f, target[0]));
		const float y = std::max(-1.f, std::min(+1.f, target[1]));
		const float z = std::max(-1.f, std::min(+1.f, target[2]));
		
		return projectToWorld(Vec3(x, y, z));
	}
};
