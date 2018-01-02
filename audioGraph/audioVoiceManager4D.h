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

#include "soundmix.h"

struct Osc4DStream;

struct AudioVoice4D : AudioVoice
{
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
	
	struct Spatialisation
	{
		Vec3 color;
		std::string name;
		int sendIndex;
		
		Vec3 pos;
		Vec3 size;
		Vec3 rot;
		Osc4D::OrientationMode orientationMode;
		Vec3 orientationCenter;
		
		SpatialCompressor spatialCompressor;
		float articulation;
		Doppler doppler;
		DistanceIntensity distanceIntensity;
		DistanceDamping distanceDampening;
		DistanceDiffusion distanceDiffusion;
		SpatialDelay spatialDelay;
		Osc4D::SubBoost subBoost;
		
		bool globalEnable;
		
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
	
	struct ReturnSideInfo
	{
		bool enabled = false;
		float distance = 100.f;
		float scatter = 0.f;
	};
	
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
	
	AudioVoice4D()
		: AudioVoice(kType_4DSOUND)
		, isSpatial(false)
		, isReturn(false)
		, spat()
		, lastSentSpat()
		, returnInfo()
	{
		spat.sendIndex = 0;
	}
};

struct AudioVoiceManager4D : AudioVoiceManager
{
	AudioMutex_Shared audioMutex;
	
	int numChannels;
	int numDynamicChannels;
	std::list<AudioVoice4D> voices;
	bool outputStereo;
	int colorIndex;
	
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
	
	void init(SDL_mutex * audioMutex, const int numChannels, const int numDynamicChannels);
	void shut();
	
	virtual bool allocVoice(AudioVoice *& voice, AudioSource * source, const char * name, const bool doRamping, const float rampDelay, const float rampTime, const int channelIndex) override;
	virtual void freeVoice(AudioVoice *& voice) override;
	
	void updateChannelIndices();
	int numDynamicChannelsUsed() const;
	
	virtual void generateAudio(float * __restrict samples, const int numSamples) override;
	
	void generateAudio(
		float * __restrict samples, const int numSamples,
		const bool doLimiting,
		const float limiterPeak,
		const OutputMode outputMode,
		const bool interleaved);
	void generateOsc(Osc4DStream & stream, const bool forceSync);
};
