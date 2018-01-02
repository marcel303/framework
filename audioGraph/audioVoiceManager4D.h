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
