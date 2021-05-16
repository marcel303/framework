#include "spatialAudioSystem-binaural.h"

// audiograph-core
#include "soundmix.h"

// framework
#include "framework.h"

// binaural
#include "binaural_oalsoft.h"

// libgg
#include "Path.h"

SpatialAudioSystem_Binaural::SpatialAudioSystem_Binaural(const char * sampleSetPath)
{
	parameterMgr.setPrefix("spatialAudioSystem");
	enabled = parameterMgr.addBool("enabled", true);
	volume = parameterMgr.addFloat("volume", 1.f);
	volume->setLimits(0.f, 1.f);
	
	std::vector<std::string> sampleSetFiles;
	{
		// list all of the mhr sample set files
		auto files = listFiles(sampleSetPath, false);
		for (auto & file : files)
			if (Path::GetExtension(file, true) == "mhr")
				sampleSetFiles.push_back(file);
		std::sort(sampleSetFiles.begin(), sampleSetFiles.end());
	}
	
	// load sample sets
	sampleSets.resize(sampleSetFiles.size());
	for (size_t i = 0; i < sampleSetFiles.size(); ++i)
	{
		loadHRIRSampleSet_Oalsoft(sampleSetFiles[i].c_str(), sampleSets[i]);
		sampleSets[i].finalize();
	}
	
	std::vector<ParameterEnum::Elem> sampleSetElems;
	for (size_t i = 0; i < sampleSetFiles.size(); ++i)
		sampleSetElems.push_back({ Path::GetBaseName(sampleSetFiles[i]).c_str(), (int)i });
	if (sampleSetElems.empty())
		sampleSetElems.push_back({ "n/a", 0 });
	sampleSetId = parameterMgr.addEnum("sampleSet", 0, sampleSetElems);
	sampleSet = &sampleSets[sampleSetId->get()];
}

void * SpatialAudioSystem_Binaural::addSource(
	const Mat4x4 & transform,
	AudioSource * audioSource,
	const float recordedDistance,
	const float headroomInDb)
{
	Source * source = new Source();
	source->transform = transform;
	source->audioSource = audioSource;
	source->recordedDistance = recordedDistance;
	source->headroomInDb = headroomInDb;
	source->binauralizer.init(sampleSet, &mutex_binaural);

	mutex_sources.lock();
	{
		sources.push_back(source);
	}
	mutex_sources.unlock();

	return source;
}

void SpatialAudioSystem_Binaural::removeSource(void *& in_source)
{
	Source * source = (Source*)in_source;

	auto i = std::find(sources.begin(), sources.end(), source);
	Assert(i != sources.end());

	mutex_sources.lock();
	{
		sources.erase(i);
	}
	mutex_sources.unlock();

	delete source;
	source = nullptr;
	
	in_source = nullptr;
}

void SpatialAudioSystem_Binaural::setSourceTransform(void * in_source, const Mat4x4 & transform)
{
	Source * source = (Source*)in_source;
	
	source->transform = transform;
}

void SpatialAudioSystem_Binaural::setListenerTransform(const Mat4x4 & transform)
{
	listenerTransform = transform;
}

void SpatialAudioSystem_Binaural::updatePanning()
{
	// Update binauralization parameters from listener and audio source transforms.
	const Mat4x4 & listenerToWorld = listenerTransform;
	const Mat4x4 worldToListener = listenerToWorld.CalcInv();

	for (auto * source : sources)
	{
		if (enabled->get())
		{
			const Mat4x4 & sourceToWorld = source->transform;
			const Vec3 sourcePosition_world = sourceToWorld.GetTranslation();
			const Vec3 sourcePosition_listener = worldToListener.Mul4(sourcePosition_world);
			float elevation;
			float azimuth;
			binaural::cartesianToElevationAndAzimuth(
					sourcePosition_listener[2],
					sourcePosition_listener[1],
					sourcePosition_listener[0],
					elevation,
					azimuth);
			const float distance = sourcePosition_listener.CalcSize();
			const float recordedDistance = source->recordedDistance;
			const float headroomInDb = source->headroomInDb;
			const float maxGain = powf(10.f, headroomInDb/20.f);
			const float normalizedDistance = distance / recordedDistance;
			const float intensity = fminf(maxGain, 1.f / (normalizedDistance * normalizedDistance + 1e-6f)) * volume->get();

			source->enabled = true;
			source->elevation = elevation;
			source->azimuth = azimuth;
			source->intensity = intensity;
		}
		else
		{
			source->enabled = false;
			source->elevation = 0.f;
			source->azimuth = 0.f;
			source->intensity = volume->get();
		}
	}
	
	if (sampleSetId->isDirty)
	{
		sampleSetId->isDirty = false;
		sampleSet = &sampleSets[sampleSetId->get()];
		
		mutex_sources.lock();
		{
			for (auto * source : sources)
			{
				if (source->binauralizer.isInit())
					source->binauralizer.shut();
				
				source->binauralizer.init(sampleSet, &mutex_binaural);
			}
		}
		mutex_sources.unlock();
	}
}

void SpatialAudioSystem_Binaural::generateLR(float * __restrict outputSamplesL, float * __restrict outputSamplesR, const int numSamples)
{
	memset(outputSamplesL, 0, numSamples * sizeof(float));
	memset(outputSamplesR, 0, numSamples * sizeof(float));

	mutex_sources.lock();
	{
		for (auto * source : sources)
		{
			ALIGN16 float inputSamples[numSamples];

			// generate source audio
			source->audioSource->generate(inputSamples, numSamples);
			
		#if AUDIO_BUFFER_VALIDATION
			for (int i = 0; i < numSamples; ++i)
				Assert(isfinite(inputSamples[i]) && fabsf(inputSamples[i]) <= 1000.f);
		#endif
			
			if (source->enabled.load())
			{
				// distance attenuation
				const float intensity = source->intensity.load();
				audioBufferRamp(inputSamples, numSamples, source->lastIntensity, intensity);
				source->lastIntensity = intensity;

				source->binauralizer.provide(inputSamples, numSamples);

				source->binauralizer.setSampleLocation(
					source->elevation.load(),
					source->azimuth.load());

				ALIGN16 float samplesL[numSamples];
				ALIGN16 float samplesR[numSamples];
				source->binauralizer.generateLR(
					samplesL,
					samplesR,
					numSamples);
				
			#if AUDIO_BUFFER_VALIDATION
				for (int i = 0; i < numSamples; ++i)
				{
					Assert(isfinite(samplesL[i]) && isfinite(samplesR[i]));
					Assert(fabsf(samplesL[i]) <= 1000.f && fabsf(samplesR[i]) <= 1000.f);
				}
			#endif
				
				audioBufferAdd(outputSamplesL, samplesL, numSamples);
				audioBufferAdd(outputSamplesR, samplesR, numSamples);
			}
			else
			{
				// volume
				const float intensity = source->intensity.load();
				audioBufferRamp(inputSamples, numSamples, source->lastIntensity, intensity);
				source->lastIntensity = intensity;
				
				audioBufferAdd(outputSamplesL, inputSamples, numSamples);
				audioBufferAdd(outputSamplesR, inputSamples, numSamples);
			}
		}
	}
	mutex_sources.unlock();
}
