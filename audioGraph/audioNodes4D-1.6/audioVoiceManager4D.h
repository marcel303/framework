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

#pragma once

#include "audioVoiceManager.h"
#include "osc4d.h"
#include "Vec3.h"

// todo : move this code to a separate 4DSOUND library

//

struct Osc4DStream;

//

extern Osc4DStream * g_oscStream;

/**
 * 4DSOUND engine version 1.6 audio voice.
 * The 4D audio voice contains various spatial parameters which can be set to control how the 4DSOUND engine
 * will process audio for the voice and pan it.
 */
struct AudioVoice4D : AudioVoice
{
	/**
	 * Spatial compression effect.
	 */
	struct SpatialCompressor
	{
		bool enable = false;
		float attack = 1.f;
		float release = 1.f;
		float minimum = 0.f;
		float maximum = 1.f;
		float curve = 0.f;
		bool invert = false;
		
		bool operator!=(const SpatialCompressor & other) const
		{
			return
				enable != other.enable ||
				attack != other.attack ||
				release != other.release ||
				minimum != other.minimum ||
				maximum != other.maximum ||
				curve != other.curve ||
				invert != other.invert;
		}
	};
	
	/**
	 * Doppler shift DSP effect.
	 */
	struct Doppler
	{
		bool enable = true;
		float scale = 1.f;
		float smooth = .2f;
		
		bool operator!=(const Doppler & other) const
		{
			return
				enable != other.enable ||
				scale != other.scale ||
				smooth != other.smooth;
		}
	};
	
	/**
	 * Distance intensity DSP effect.
	 */
	struct DistanceIntensity
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
		
		bool operator!=(const DistanceIntensity & other) const
		{
			return
				enable != other.enable ||
				threshold != other.threshold ||
				curve != other.curve;
		}
	};
	
	/**
	 * Distance damping DSP effect.
	 */
	struct DistanceDamping
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
		
		bool operator!=(const DistanceDamping & other) const
		{
			return
				enable != other.enable ||
				threshold != other.threshold ||
				curve != other.curve;
		}
	};
	
	/**
	 * Distance diffusion DSP effect.
	 */
	struct DistanceDiffusion
	{
		bool enable = false;
		float threshold = 0.f;
		float curve = 0.f;
		
		bool operator!=(const DistanceDiffusion & other) const
		{
			return
				enable != other.enable ||
				threshold != other.threshold ||
				curve != other.curve;
		}
	};
	
	/**
	 * Spatial delay DSP effect.
	 */
	struct SpatialDelay
	{
		bool enable = false;
		Osc4D::SpatialDelayMode mode = Osc4D::kSpatialDelay_Grid;
		float feedback = .5f;
		float wetness = .5f;
		float smooth = 0.f;
		float scale = 1.f;
		float noiseDepth = 0.f;
		float noiseFrequency = 1.f;
		
		bool operator!=(const SpatialDelay & other) const
		{
			return
				enable != other.enable ||
				mode != other.mode ||
				feedback != other.feedback ||
				wetness != other.wetness ||
				smooth != other.smooth ||
				scale != other.scale ||
				noiseDepth != other.noiseDepth ||
				noiseFrequency != other.noiseFrequency;
		}
	};
	
	/**
	 * All spatial 'sound object' parameters.
	 */
	struct Spatialisation
	{
		Vec3 color; ///< Color of the sound object inside the monitor.
		std::string name; ///< Name of the sound object inside the monitor.
		int sendIndex; ///< Channel index of the 'return' channel.
		
		Vec3 pos; ///< Position.
		Vec3 size; ///< Dimensions.
		Vec3 rot; ///< Euler rotation.
		Osc4D::OrientationMode orientationMode;
		Vec3 orientationCenter;
		
		SpatialCompressor spatialCompressor;
		float articulation; ///< Grid panner specific parameter to control how smooth or not a source is panned across speakers.
		Doppler doppler;
		DistanceIntensity distanceIntensity;
		DistanceDamping distanceDampening;
		DistanceDiffusion distanceDiffusion;
		SpatialDelay spatialDelay;
		Osc4D::SubBoost subBoost;
		
		bool globalEnable; ///< Global transformations are enabled for this voice when true.
		
		Spatialisation()
			: color(1.f, 0.f, 0.f)
			, name()
			, sendIndex(-1)
			, pos()
			, size(1.f, 1.f, 1.f)
			, rot()
			, orientationMode(Osc4D::kOrientation_Static)
			, orientationCenter(0.f, 2.f, 0.f)
			, spatialCompressor()
			, articulation(0.f)
			, doppler()
			, distanceIntensity()
			, distanceDampening()
			, distanceDiffusion()
			, spatialDelay()
			, subBoost(Osc4D::kBoost_None)
			, globalEnable(true)
		{
		}
	};
	
	/**
	 * Information regarding projecting sound from the 'return' channel back into the engine.
	 */
	struct ReturnSideInfo
	{
		bool enabled = false;
		float distance = 100.f;
		float scatter = 0.f;
	};
	
	/**
	 * Information regarding projecting sound from the 'return' channel back into the engine.
	 */
	struct ReturnInfo
	{
		int returnIndex;
		
		ReturnSideInfo sides[Osc4D::kReturnSide_COUNT];
		
		ReturnInfo()
			: returnIndex(-1)
			, sides()
		{
		}
	};

	bool isSpatial;
	bool isReturn;
	
	Spatialisation spat;
	Spatialisation lastSentSpat;
	
	ReturnInfo returnInfo;
	
	bool initOsc; ///< When set to true, this will trigger a sync of all spatialization parameters across OSC.
	
	AudioVoice4D()
		: AudioVoice(kType_4DSOUND)
		, isSpatial(false)
		, isReturn(false)
		, spat()
		, lastSentSpat()
		, returnInfo()
		, initOsc(true)
	{
		spat.sendIndex = 0;
	}
};

/**
 * 4DSOUND engine version 1.6 implementation of AudioVoiceManager.
 * The 4D voice manager creates and maintains 4DSOUND voices, and sends OSC updates
 * when spatial parameters for voices and globals are changed.
 */
struct AudioVoiceManager4D : AudioVoiceManager
{
private:
	AudioMutex_Shared audioMutex;
	
	AudioVoice * firstVoice;
	int colorIndex;
	
	int numDynamicChannels;
	
	Osc4DStream * oscStream;

public:
	bool outputStereo;
	
	/**
	 * Global spatialisation parameters.
	 */
	struct Spatialisation
	{
		float globalGain;
		Vec3 globalPos;
		Vec3 globalSize;
		Vec3 globalRot;
		Vec3 globalPlode;
		Vec3 globalOrigin;
		
		Spatialisation()
			: globalGain(1.f)
			, globalPos()
			, globalSize(1.f, 1.f, 1.f)
			, globalRot()
			, globalPlode(1.f, 1.f, 1.f)
			, globalOrigin()
		{
		}
	};
	
	Spatialisation spat;
	Spatialisation lastSentSpat;
	
	AudioVoiceManager4D();
	
	void init(SDL_mutex * audioMutex, const int numDynamicChannels, const char * ipAddress, const int udpPort);
	void shut();
	
	virtual bool allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex = -1) override;
	virtual void freeVoice(AudioVoice *& voice) override;
	virtual int calculateNumVoices() const override;
	
	int calculateNumDynamicChannelsUsed() const;
	int getNumDynamicChannels() const;
	
	virtual void generateAudio(float * __restrict samples, const int numSamples, const int numChannels) override;
	
	void generateAudio(
		float * __restrict samples, const int numSamples, const int numChannels,
		const bool doLimiting,
		const float limiterPeak,
		const bool updateRamping,
		const OutputMode outputMode,
		const bool interleaved);
	
	/**
	 * Generates OSC messages for voices and global parameters that changed, or for all parameters when forceSync is set to true.
	 * @param stream The stream to which to output OSC messages.
	 * @param forceSync When true, OSC messages will be sent for all spatialization parameters, not just the ones that changed. This can be helpful when the spatialization engine is started after the app, or for debugging purposes. Note it's generally not recommended to always set this to true, as it creates a lot of OSC traffic.
	 */
	void generateOsc(Osc4DStream & stream, const bool forceSync);

	void setOscEndpoint(const char * ipAddress, const int udpPort);
	
private:
	void updateDynamicChannelIndices();
};
