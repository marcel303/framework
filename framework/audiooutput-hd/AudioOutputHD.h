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

class AudioStreamHD
{
public:
	struct StreamInfo
	{
		float  frameRate;        // number of (single- or mult-channel) frames that are output per second
		double secondsSincePlay; // the number of seconds since Play() was called
		int    framesSincePlay;  // the number of frames since Play() was called
		float  outputLatency;    // audio output latency in seconds. use this for precision timing when for instance scheduling notes
	};
	
	struct ProvideInfo
	{
		const float ** inputSamples;
		int numInputChannels;
		float ** outputSamples;
		int numOutputChannels;
		int numFrames;
	};
	
	virtual int Provide(
		const ProvideInfo & provideInfo,
		const StreamInfo & streamInfo) = 0;
};

class AudioOutputHD
{
public:
	virtual ~AudioOutputHD() { }
	
	virtual bool Initialize(
		const int numInputChannels,
		const int numOutputChannels,
		const int frameRate,
		const int bufferSize) = 0;
	virtual bool Shutdown() = 0;
	
	virtual void Play(AudioStreamHD * stream) = 0;
	virtual void Stop() = 0;
	
	virtual void  Volume_set(const float volume) = 0;
	virtual float Volume_get() const = 0;
	
	void  GainDb_set(const float gainDb);
	float GainDb_get() const;
	
	virtual bool IsPlaying_get() const = 0;
	
	virtual int BufferSize_get() const = 0;
	virtual int FrameRate_get() const = 0;
};
