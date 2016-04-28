#include "audiofft.h"
#include "AudioFFTLib.h"
#include "audiostream/AudioStream.h"
#include "Calc.h"

static audiofft::AudioFFT s_fft;

float s_fftInputBuffer[4096] = { };
float s_fftInput[kFFTSize] = { };
float s_fftReal[kFFTComplexSize] = { };
float s_fftImaginary[kFFTComplexSize] = { };
float s_fftBuckets[kFFTBucketCount] = { };

float s_fftProvideTime = 0.f;

void fftInit()
{
	s_fft.init(kFFTSize);
}

void fftShutdown()
{
	s_fft.init(0);

	memset(s_fftInputBuffer, 0, sizeof(s_fftInputBuffer));
	memset(s_fftInput, 0, sizeof(s_fftInput));
	memset(s_fftReal, 0, sizeof(s_fftReal));
	memset(s_fftImaginary, 0, sizeof(s_fftImaginary));
	memset(s_fftBuckets, 0, sizeof(s_fftBuckets));

	s_fftProvideTime = 0.f;
}

float fftPowerValue(int i)
{
	float p = s_fftReal[i] * s_fftReal[i] + s_fftImaginary[i] * s_fftImaginary[i];
#if 1
	p = sqrtf(p);
#else
	p = 10.f * std::log10f(p);
#endif
	return p;
}

void fftProcess(float time)
{
	const float dt = time - s_fftProvideTime;
	int sampleStart = dt * 44100.f; // fixme
	if (sampleStart + kFFTSize > kFFTBufferSize)
		sampleStart = kFFTBufferSize - kFFTSize;

	//sampleStart = 0;

	s_fft.fft(s_fftInputBuffer + sampleStart, s_fftReal, s_fftImaginary);

	for (int i = 0; i < kFFTBucketCount; ++i)
	{
		const int numSamples = kFFTComplexSize / kFFTBucketCount;
		const int j1 = (i + 0) * numSamples;
		const int j2 = (i + 1) * numSamples;
		//Assert(j2 <= kFFTSize);

		float result = 0.f;

		for (int j = j1; j < j2; ++j)
			result += fftPowerValue(j);

		s_fftBuckets[i] = result / numSamples;
	}
}

void fftProvide(AudioSample * __restrict samples, int numSamples, float provideTime)
{
	const int copySize = Calc::Min(numSamples, kFFTBufferSize);
	const float scale = 2.f / 65536.f / 2.f; // divide by two to compensate for stereo samples

	for (int i = 0; i < copySize; ++i)
		s_fftInputBuffer[i] = (samples[i].channel[0] + samples[i].channel[1]) * scale;
	for (int i = copySize; i < kFFTBufferSize; ++i)
		s_fftInputBuffer[i] = 0.f;

	s_fftProvideTime = provideTime;
}