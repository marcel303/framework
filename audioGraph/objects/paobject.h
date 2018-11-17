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

typedef void PaStream;

struct PortAudioHandler
{
	virtual ~PortAudioHandler()
	{
	}
	
	virtual void portAudioCallback(
		const void * inputBuffer,
		const int numInputChannels,
		void * outputBuffer,
		const int framesPerBuffer) = 0;
};

struct PortAudioObject
{
	bool paInitialized;
	PaStream * stream;
	PortAudioHandler * handler;
	int numOutputChannels;
	int numInputChannels;
	
	PortAudioObject()
		: paInitialized(false)
		, stream(nullptr)
		, handler(nullptr)
		, numOutputChannels(0)
		, numInputChannels(0)
	{
	}
	
	~PortAudioObject()
	{
		shut();
	}
	
	bool findSupportedDevices(const int numInputChannels, const int numOutputChannels, int & inputDeviceIndex, int & outputDeviceIndex) const;
	bool isSupported(const int numInputChannels, const int numOutputChannels) const;
	
	bool init(const int sampleRate, const int numOutputChannels, const int numInputChannels, const int bufferSize, PortAudioHandler * audioSource, const int inputDeviceIndex = -1, const int outputDeviceIndex = -1, const bool useFloatFormat = true);
	bool initImpl(const int sampleRate, const int numOutputChannels, const int numInputChannels, const int bufferSize, PortAudioHandler * audioSource, const int inputDeviceIndex, const int outputDeviceIndex, const bool useFloatFormat);
	bool shut();
	
	double getCpuUsage() const;
};
