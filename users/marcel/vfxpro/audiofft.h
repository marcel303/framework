#pragma once

struct AudioSample;

// FFT

static const int kFFTBufferSize = 4096;
static const int kFFTSize = 1024;
static const int kFFTComplexSize = 513; // n/2+1
static const int kFFTBucketCount = 32;

extern float s_fftInputBuffer[4096];
extern float s_fftInput[kFFTSize];
extern float s_fftReal[kFFTComplexSize];
extern float s_fftImaginary[kFFTComplexSize];
extern float s_fftBuckets[kFFTBucketCount];

extern float s_fftProvideTime;

void fftInit();
void fftShutdown();
float fftPowerValue(int i);
void fftProcess(float time);
void fftProvide(const AudioSample * __restrict samples, int numSamples, float provideTime);
