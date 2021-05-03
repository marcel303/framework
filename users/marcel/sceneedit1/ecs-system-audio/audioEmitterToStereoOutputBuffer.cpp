#include "audioEmitterToStereoOutputBuffer.h"

// ecs-system-audio
#include "audioEmitterComponent.h"
#include "reverbZoneComponent.h"

// ecs-scene
#include "sceneNodeComponent.h"

// zita-rev1
#include "zita-rev1.h"

#define ENABLE_AUDIO_SOURCE_HACK 1

#if ENABLE_AUDIO_SOURCE_HACK
#include "audiostream/AudioStreamWave.h" // todo : remove
#include "framework.h" // todo : remove

static bool s_isInit = false;
static AudioStreamWave s_audioStream;
static AudioSample s_audioStreamSamples[256];
#endif

static float computeDistanceSqToReverbZone(
	Vec3Arg position_world,
	const ReverbZoneComponentMgr::Zone & reverbZone)
{
	// determine distance to world space position based on volume of the reverb zone
	
	const Vec3 position_reverbZone = reverbZone.worldToObject.Mul4(position_world).Div(reverbZone.boxExtents);
	
// todo : compute in reverb zone space or world space ? pro : distance attenuation scales with reverb zone size. con : not sure if that's actually correct ..
	const Vec3 nearestPointOnBox_reverbZone = position_reverbZone.Max(Vec3(-1.f)).Min(Vec3(+1.f));
	const Vec3 delta_reverbZone = position_reverbZone - nearestPointOnBox_reverbZone;
	
	return delta_reverbZone.CalcSizeSq();
}

void audioEmitterToStereoOutputBuffer(
	const binaural::HRIRSampleSet * hrirSampleSet,
	const Mat4x4 & worldToListener,
	float * __restrict outputBufferL,
	float * __restrict outputBufferR,
	const int numFramesThisBlock)
{
	const Mat4x4 listenerToWorld = worldToListener.CalcInv();
	const Vec3 listenerPosition_world = listenerToWorld.GetTranslation();
	
// todo : use eight corners plus nearest vertex method for spatializing reverb zones? or rethink the issue

#if ENABLE_AUDIO_SOURCE_HACK
// todo : remove wave sample hack
	if (s_isInit == false)
	{
		s_isInit = true;
		auto * resolved_path = framework.resolveResourcePath("snare.wav");
		s_audioStream.Open(resolved_path, true);
	}
	s_audioStream.Provide(numFramesThisBlock, s_audioStreamSamples); // todo : remove
#endif

	// produce samples for audio emitter
	
	for (int a = 0; a < g_audioEmitterComponentMgr.emitters.size(); ++a)
	{
		auto & audioEmitter = g_audioEmitterComponentMgr.emitters[a];
		
		if (audioEmitter.enabled == false)
			continue;
		
	#if ENABLE_AUDIO_SOURCE_HACK
		for (int s = 0; s < numFramesThisBlock; ++s)
		{
			//audioEmitter.outputBuffer[s] = (rand() / float(RAND_MAX) - .5f) * .4f;
			audioEmitter.outputBuffer[s] = (s_audioStreamSamples[s].channel[0] + s_audioStreamSamples[s].channel[1]) / float(1 << 16);
		}
	#endif
		
		audioEmitter.binauralizer->setSampleSet(hrirSampleSet);
	}
	
	// compute routing weights for each audio emitter vs each reverb zone,
	// and insert the weight + reverb zone into a (limited capacity) list
	// of reverb zones per audio emitter
	
	const int kMaxReverbZonesPerAudioEmitter = 4;
	
	int   * __restrict reverbZoneIndices = (int*)  alloca(g_audioEmitterComponentMgr.numComponents * kMaxReverbZonesPerAudioEmitter * sizeof(int));
	float * __restrict reverbZoneWeights = (float*)alloca(g_audioEmitterComponentMgr.numComponents * kMaxReverbZonesPerAudioEmitter * sizeof(float));
	
	for (int i = 0; i < g_audioEmitterComponentMgr.numComponents * kMaxReverbZonesPerAudioEmitter; ++i)
	{
		reverbZoneIndices[i] = -1;
	}
	
	int   * __restrict reverbZoneIndices_itr = reverbZoneIndices;
	float * __restrict reverbZoneWeights_itr = reverbZoneWeights;
	
	for (int a = 0; a < g_audioEmitterComponentMgr.emitters.size();
		++a,
		reverbZoneIndices_itr += kMaxReverbZonesPerAudioEmitter,
		reverbZoneWeights_itr += kMaxReverbZonesPerAudioEmitter)
	{
		auto & audioEmitter = g_audioEmitterComponentMgr.emitters[a];
		
		if (audioEmitter.enabled == false || audioEmitter.hasTransform == false)
			continue;
		
		for (int z = 0; z < g_reverbZoneComponentMgr.zones.size(); ++z)
		{
			auto & reverbZone = g_reverbZoneComponentMgr.zones[z];
			
			if (reverbZone.enabled == false || reverbZone.hasTransform == false)
				continue;
				
			// compute reverb zone weight depending on audio emitter position and reverb zone geometry
		
			const Vec3 audioEmitterPosition_world = audioEmitter.objectToWorld.GetTranslation();
		
			const float attenuationDistanceSq = computeDistanceSqToReverbZone(audioEmitterPosition_world, reverbZone);
			
			const float reverbZoneWeight = 1.f / fmaxf(1.f, attenuationDistanceSq);
			
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
		
		// normalize weight, so the audio emitter distributes its audio signal amongst the reverb zones
		
		float totalWeight = 0.f;
		
		for (int i = 0; i < kMaxReverbZonesPerAudioEmitter; ++i)
			if (reverbZoneIndices_itr[i] != -1)
				totalWeight += reverbZoneWeights_itr[i];
		
		if (totalWeight > 0.f)
		{
			for (int i = 0; i < kMaxReverbZonesPerAudioEmitter; ++i)
				if (reverbZoneIndices_itr[i] != -1)
					reverbZoneWeights_itr[i] /= totalWeight;
		}
	}
	
	// perform reverb processing
	
	// 1. initialize the reverb input buffer to zero
	
	for (int z = 0; z < g_reverbZoneComponentMgr.zones.size(); ++z)
	{
		auto & reverbZone = g_reverbZoneComponentMgr.zones[z];
		
		if (reverbZone.enabled == false)
			continue;
		
		for (int i = 0; i < numFramesThisBlock; ++i)
		{
			reverbZone.inputBuffer[i] = 0.f;
		}
	}
	
	// 2. accumulate audio emitter output buffers multiplied by weight into reverb zone input buffers
	
	reverbZoneIndices_itr = reverbZoneIndices;
	reverbZoneWeights_itr = reverbZoneWeights;
	
	for (int a = 0; a < g_audioEmitterComponentMgr.emitters.size();
		++a,
		reverbZoneIndices_itr += kMaxReverbZonesPerAudioEmitter,
		reverbZoneWeights_itr += kMaxReverbZonesPerAudioEmitter)
	{
		auto & audioEmitter = g_audioEmitterComponentMgr.emitters[a];
		
		if (audioEmitter.enabled == false)
			continue;
		
		for (int i = 0; i < kMaxReverbZonesPerAudioEmitter; ++i)
		{
			if (reverbZoneIndices_itr[i] != -1)
			{
				const int   reverbZoneIndex  = reverbZoneIndices_itr[i];
				const float reverbZoneWeight = reverbZoneWeights_itr[i];
				
				auto & reverbZone = g_reverbZoneComponentMgr.zones[reverbZoneIndex];
				
				for (int s = 0; s < numFramesThisBlock; ++s)
				{
					reverbZone.inputBuffer[s] += audioEmitter.outputBuffer[s] * reverbZoneWeight;
				}
			}
		}
	}
	
	// 3. process reverb and accumulate into left-right input buffers
	
	float reverbOutputBufferL[numFramesThisBlock];
	float reverbOutputBufferR[numFramesThisBlock];
	
	memset(reverbOutputBufferL, 0, numFramesThisBlock * sizeof(float));
	memset(reverbOutputBufferR, 0, numFramesThisBlock * sizeof(float));
	
	for (int z = 0; z < g_reverbZoneComponentMgr.zones.size(); ++z)
	{
		auto & reverbZone = g_reverbZoneComponentMgr.zones[z];
		
		if (reverbZone.enabled == false)
			continue;
		
		reverbZone.reverb->prepare(numFramesThisBlock);
		
		float * src[2] =
		{
			reverbZone.inputBuffer,
			reverbZone.inputBuffer
		};
		
		float samplesL[numFramesThisBlock];
		float samplesR[numFramesThisBlock];
		
		float * dst[2] =
		{
			samplesL,
			samplesR
		};
		
		auto & reverb = *reverbZone.reverb;
		
		reverb.process(numFramesThisBlock, src, dst);
		
		// calculate distance attenuation
		
		const float attenuationDistanceSq = computeDistanceSqToReverbZone(listenerPosition_world, reverbZone);
		
		const float distanceAttenuation = 1.f / fmaxf(1.f, attenuationDistanceSq);
		
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
	
	for (int a = 0; a < g_audioEmitterComponentMgr.emitters.size(); ++a)
	{
		auto & audioEmitter = g_audioEmitterComponentMgr.emitters[a];
		
		if (audioEmitter.enabled == false || audioEmitter.hasTransform == false)
			continue;
		
		// determine audio emitter position relative to the listener and compute elevation and azimuth for binauralization
		
		const Vec3 audioEmitterPosition_world = audioEmitter.objectToWorld.GetTranslation();
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
		
		audioEmitter.binauralizer->setSampleLocation(elevation, azimuth);
		
		audioEmitter.binauralizer->provide(audioEmitter.outputBuffer, numFramesThisBlock);
		
		// binauralize input signal
		
		float samplesL[numFramesThisBlock];
		float samplesR[numFramesThisBlock];
		
		audioEmitter.binauralizer->generateLR(samplesL, samplesR, numFramesThisBlock);
		
		// calculate distance attenuation
		
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
}
