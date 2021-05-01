#include "audioEmitterToAudioOutput.h"

#include "audioEmitterComponent.h"
#include "reverbZoneComponent.h"

#include "sceneNodeComponent.h"

#include "binaural_oalsoft.h" // todo : move to other file
#include "framework.h" // framework.resolveResourcePath // todo : remove. let entity initializing audio system do the resolve

#include "audiostream/AudioStreamWave.h"

static binaural::HRIRSampleSet * s_hrirSampleSet = nullptr;

static AudioStreamWave s_audioStream;
static AudioSample s_audioStreamSamples[256];

void audioEmitterToStereoOutputBuffer(
	const Mat4x4 & worldToListener,
	float * __restrict outputBufferL,
	float * __restrict outputBufferR,
	const int numFramesThisBlock)
{
// todo : use eight corners plus nearest vertex method for spatializing reverb zones? or rethink the issue

// todo : add a central place to load the HRIR sample set
	if (s_hrirSampleSet == nullptr)
	{
		s_hrirSampleSet = new binaural::HRIRSampleSet();
	// fixme : resolve on audio thread is not safe
		auto * resolved_path = framework.resolveResourcePath("ecs-system-audio/binaural/Default HRTF.mhr");
		binaural::loadHRIRSampleSet_Oalsoft(resolved_path, *s_hrirSampleSet);
		s_hrirSampleSet->finalize();
		
		resolved_path = framework.resolveResourcePath("snare.wav");
		s_audioStream.Open(resolved_path, true);
	}
	s_audioStream.Provide(numFramesThisBlock, s_audioStreamSamples); // todo : remove

	g_audioEmitterComponentMgr.mutex.lock();
	g_reverbZoneComponentMgr.mutex.lock();

	// produce samples for audio emitter
	
	for (int a = 0; a < g_audioEmitterComponentMgr.numComponents; ++a)
	{
		auto * audioEmitterComp = g_audioEmitterComponentMgr.components[a];
		
		if (audioEmitterComp == nullptr || audioEmitterComp->enabled == false)
			continue;
		
		for (int s = 0; s < numFramesThisBlock; ++s)
		{
			//audioEmitterComp->outputBuffer[s] = (rand() / float(RAND_MAX) - .5f) * .4f;
			audioEmitterComp->outputBuffer[s] = (s_audioStreamSamples[s].channel[0] + s_audioStreamSamples[s].channel[1]) / float(1 << 16);
		}
		
	// todo : add method to change HRIR sample set?
		audioEmitterComp->binauralizer.shut();
		audioEmitterComp->binauralizer.init(s_hrirSampleSet, &audioEmitterComp->mutex);
	}
	
	// compute routing weights for each audio emitter vs each reverb zone,
	// and insert the weight + reverb zone into a (limited capacity) list
	// of reverb zones per audio emitter
	
	const int kMaxReverbZonesPerAudioEmitter = 2;
	
	int   * __restrict reverbZoneIndices = (int*)  alloca(g_audioEmitterComponentMgr.numComponents * kMaxReverbZonesPerAudioEmitter * sizeof(int));
	float * __restrict reverbZoneWeights = (float*)alloca(g_audioEmitterComponentMgr.numComponents * kMaxReverbZonesPerAudioEmitter * sizeof(float));
	
	for (int i = 0; i < g_audioEmitterComponentMgr.numComponents * kMaxReverbZonesPerAudioEmitter; ++i)
	{
		reverbZoneIndices[i] = -1;
	}
	
	int   * __restrict reverbZoneIndices_itr = reverbZoneIndices;
	float * __restrict reverbZoneWeights_itr = reverbZoneWeights;
	
	for (int a = 0; a < g_audioEmitterComponentMgr.numComponents;
		++a,
		reverbZoneIndices_itr += kMaxReverbZonesPerAudioEmitter,
		reverbZoneWeights_itr += kMaxReverbZonesPerAudioEmitter)
	{
		auto * audioEmitterComp = g_audioEmitterComponentMgr.components[a];
		
		if (audioEmitterComp == nullptr || audioEmitterComp->enabled == false)
			continue;
		
		for (int z = 0; z < g_reverbZoneComponentMgr.numComponents; ++z)
		{
			auto * reverbZoneComp = g_reverbZoneComponentMgr.components[z];
			
			if (reverbZoneComp == nullptr || reverbZoneComp->enabled == false)
				continue;
				
		// todo : compute reverb zone weight depending on audio emitter position and reverb zone geometry
		
			const float reverbZoneWeight = 1.f;
			
			if (reverbZoneWeight <= 0.f)
				continue;;
				
			int   slot_index = -1;
			float slot_weight = reverbZoneWeight;
			
			for (int i = 0; i < kMaxReverbZonesPerAudioEmitter; ++i)
			{
				// insert the reverb zone into the list of reverb zones per audio emitter,
				// depending on whether its weight is greater than zero and greater than
				// the weight of any other item in the list or if there's still an empty
				// spot left in the list
				
				if (reverbZoneIndices_itr[i] == -1)
				{
					slot_index = i;
					break;
				}
				
				if (reverbZoneWeights_itr[i] < slot_weight)
				{
					slot_index = i;
					slot_weight = reverbZoneWeights_itr[i];
				}
			}
			
			if (slot_index != -1)
			{
				reverbZoneIndices_itr[slot_index] = z;
				reverbZoneWeights_itr[slot_index] = reverbZoneWeight;
			}
		}
	}
	
	// perform reverb processing
	
	// 1. initialize the reverb input buffer to zero
	
	for (int z = 0; z < g_reverbZoneComponentMgr.numComponents; ++z)
	{
		auto * reverbZoneComp = g_reverbZoneComponentMgr.components[z];
		
		if (reverbZoneComp == nullptr || reverbZoneComp->enabled == false)
			continue;
		
		for (int i = 0; i < numFramesThisBlock; ++i)
		{
			reverbZoneComp->inputBuffer[0][i] = 0.f;
			reverbZoneComp->inputBuffer[1][i] = 0.f;
		}
	}
	
	// 2. accumulate audio emitter output buffers multiplied by weight into reverb zone input buffers
	
	reverbZoneIndices_itr = reverbZoneIndices;
	reverbZoneWeights_itr = reverbZoneWeights;
	
	for (int a = 0; a < g_audioEmitterComponentMgr.numComponents;
		++a,
		reverbZoneIndices_itr += kMaxReverbZonesPerAudioEmitter,
		reverbZoneWeights_itr += kMaxReverbZonesPerAudioEmitter)
	{
		auto * audioEmitterComp = g_audioEmitterComponentMgr.components[a];
		
		if (audioEmitterComp == nullptr || audioEmitterComp->enabled == false)
			continue;
		
		for (int i = 0; i < kMaxReverbZonesPerAudioEmitter; ++i)
		{
			if (reverbZoneIndices_itr[i] != -1)
			{
				const int   reverbZoneIndex  = reverbZoneIndices_itr[i];
				const float reverbZoneWeight = reverbZoneWeights_itr[i];
				
				auto * reverbZoneComp = g_reverbZoneComponentMgr.components[reverbZoneIndex];
				
				for (int s = 0; s < numFramesThisBlock; ++s)
				{
					reverbZoneComp->inputBuffer[0][s] += audioEmitterComp->outputBuffer[s] * reverbZoneWeight;
					reverbZoneComp->inputBuffer[1][s] += audioEmitterComp->outputBuffer[s] * reverbZoneWeight;
				}
			}
		}
	}
	
	// 3. process reverb and accumulate into left-right input buffers
	
	float reverbOutputBufferL[numFramesThisBlock];
	float reverbOutputBufferR[numFramesThisBlock];
	
	memset(reverbOutputBufferL, 0, numFramesThisBlock * sizeof(float));
	memset(reverbOutputBufferR, 0, numFramesThisBlock * sizeof(float));
	
	for (int z = 0; z < g_reverbZoneComponentMgr.numComponents; ++z)
	{
		auto * reverbZoneComp = g_reverbZoneComponentMgr.components[z];
		
		if (reverbZoneComp == nullptr || reverbZoneComp->enabled == false)
			continue;
		
		reverbZoneComp->reverb.prepare(numFramesThisBlock);
		
		float * src[2] =
		{
			reverbZoneComp->inputBuffer[0],
			reverbZoneComp->inputBuffer[1]
		};
		
		float samplesL[numFramesThisBlock];
		float samplesR[numFramesThisBlock];
		
		float * dst[2] =
		{
			samplesL,
			samplesR
		};
		
		// todo : let zita-rev1 perform accumulation into dst
		
		auto & reverb = reverbZoneComp->reverb;
		
		reverb.set_delay(reverbZoneComp->preDelay);
		reverb.set_rtlow(reverbZoneComp->t60Low);
		reverb.set_rtmid(reverbZoneComp->t60Mid);
		reverb.set_xover(reverbZoneComp->t60CrossoverFrequency);
		reverb.set_fdamp(reverbZoneComp->dampingFrequency);
		reverb.set_eq1( 160.f, reverbZoneComp->eq1Gain);
		reverb.set_eq2(2500.f, reverbZoneComp->eq2Gain);
		
		reverb.process(numFramesThisBlock, src, dst);
		
		// calculate distance attenuation
		
		auto * sceneNodeComp = reverbZoneComp->componentSet->find<SceneNodeComponent>();
		
	// todo : determine distance to listener based on volume of the reverb zone
		const Vec3 reverbZonePosition_world = sceneNodeComp->objectToWorld.GetTranslation();
		const Vec3 reverbZonePosition_listener = worldToListener.Mul4(reverbZonePosition_world);
		
		const float distanceToListenerSq = reverbZonePosition_listener.CalcSizeSq();
		
		const float kMinDistanceSq = powf(.1f, 2.f);
		const float distanceAttenuation = 1.f / fmaxf(kMinDistanceSq, distanceToListenerSq);
		
		// apply distance attenuation and accumulate into reverb output buffer
		
		for (int s = 0; s < numFramesThisBlock; ++s)
		{
			reverbOutputBufferL[s] += samplesL[s] * distanceAttenuation;
			reverbOutputBufferR[s] += samplesR[s] * distanceAttenuation;
		}
	}
	
	// binauralize the signal for each audio emitter and accumulate the stereo output signals
	
	float binauralOutputBufferL[numFramesThisBlock];
	float binauralOutputBufferR[numFramesThisBlock];
	
	memset(binauralOutputBufferL, 0, numFramesThisBlock * sizeof(float));
	memset(binauralOutputBufferR, 0, numFramesThisBlock * sizeof(float));
	
	for (int a = 0; a < g_audioEmitterComponentMgr.numComponents; ++a)
	{
		auto * audioEmitterComp = g_audioEmitterComponentMgr.components[a];
		
		if (audioEmitterComp == nullptr || audioEmitterComp->enabled == false)
			continue;
		
		// determine audio emitter position relative to the listener and compute elevation and azimuth for binauralization
		
		auto * sceneNodeComp = audioEmitterComp->componentSet->find<SceneNodeComponent>();
		
		const Vec3 audioEmitterPosition_world = sceneNodeComp->objectToWorld.GetTranslation();
		const Vec3 audioEmitterPosition_listener = worldToListener.Mul4(audioEmitterPosition_world);
		
		float elevation;
		float azimuth;
		binaural::cartesianToElevationAndAzimuth(
			audioEmitterPosition_listener[2],
			audioEmitterPosition_listener[1],
			audioEmitterPosition_listener[0],
			elevation,
			azimuth);
		
		//logDebug("elevation: %.2f, azimuth: %.2f", elevation, azimuth);
		
		// set elevation and azimuth and give input samples to the binauralizer object
		
		audioEmitterComp->binauralizer.setSampleLocation(elevation, azimuth);
		
		audioEmitterComp->binauralizer.provide(audioEmitterComp->outputBuffer, numFramesThisBlock);
		
		// binauralize input signal
		
		float samplesL[numFramesThisBlock];
		float samplesR[numFramesThisBlock];
		
		audioEmitterComp->binauralizer.generateLR(samplesL, samplesR, numFramesThisBlock);
		
		// calculate distance attenuation
		
	// todo : determine distance to listener based on volume of the audio emitter
		const float distanceToListenerSq = audioEmitterPosition_listener.CalcSizeSq();
		
		const float kMinDistanceSq = powf(.1f, 2.f);
		const float distanceAttenuation = 1.f / fmaxf(kMinDistanceSq, distanceToListenerSq);
		
		// apply distance attenuation and accumulate into binaural output buffer
		
		for (int s = 0; s < numFramesThisBlock; ++s)
		{
			binauralOutputBufferL[s] += samplesL[s] * distanceAttenuation;
			binauralOutputBufferR[s] += samplesR[s] * distanceAttenuation;
		}
	}
	
	// accumulate reverb left-right and binaural left-right into the left-right output buffers
	
	for (int s = 0; s < numFramesThisBlock; ++s)
	{
		outputBufferL[s] = reverbOutputBufferL[s] + binauralOutputBufferL[s];
		outputBufferR[s] = reverbOutputBufferR[s] + binauralOutputBufferR[s];
	}
	
	g_reverbZoneComponentMgr.mutex.unlock();
	g_audioEmitterComponentMgr.mutex.unlock();
}
