#include "framework.h"
#include "vfxNodes/fourier.h"
#include <complex>

#define SAMPLE_BUFFER_SIZE 512
#define SAMPLE_RATE 44100

struct AudioBuffer
{
	float r[SAMPLE_BUFFER_SIZE];
	float i[SAMPLE_BUFFER_SIZE];
	
	void transform(const bool forward)
	{
		Fourier::fft1D_slow(r, i, SAMPLE_BUFFER_SIZE, SAMPLE_BUFFER_SIZE, forward == false, false);
	}
	
	void convolve(const AudioBuffer & filter, AudioBuffer & output)
	{
		for (int x = 0; x < SAMPLE_BUFFER_SIZE; ++x)
		{
			// todo : perform complex multiply
			
			std::complex<float> a(r[x], i[x]);
			std::complex<float> b(filter.r[x], filter.i[x]);
			std::complex<float> r = a * b;
			
			output.r[x] = r.real();
			output.i[x] = r.imag();
		}
	}
};

static void convolveAudio(AudioBuffer & source, const AudioBuffer & lFilter, const AudioBuffer & rFilter, AudioBuffer & lResult, AudioBuffer & rResult)
{
	// todo : transform audio data from the time-domain into the frequency-domain
	
	source.transform(true);
	
	// todo : convolve audio data with impulse-response data in the frequency-domain
	
	source.convolve(lFilter, lResult);
	source.convolve(rFilter, rResult);
	
	// todo : transform convolved audio data back to the time-domain
	
	lResult.transform(false);
	rResult.transform(false);
}

static float phase = 0.f;
static float phaseStep = 400.f / SAMPLE_RATE;

static AudioBuffer lFilters[1];
static AudioBuffer rFilters[1];

static void audioCallback()
{
	AudioBuffer source;
	
	for (int i = 0; i < SAMPLE_BUFFER_SIZE; ++i)
	{
		source.r[i] = std::sin(phase);
		source.i[i] = 0.f;
		
		phase = std::fmodf(phase + phaseStep, 1.f);
	}
	
	const AudioBuffer & lFilter = lFilters[0];
	const AudioBuffer & rFilter = rFilters[0];
	
	AudioBuffer lResult;
	AudioBuffer rResult;
	
	convolveAudio(source, lFilter, rFilter, lResult, rResult);
}

void testHrtf()
{
	// todo : load impulse-response audio files
	
	// todo : transform impulse-response data from the time-domain into the frequency-domain
	
	do
	{
		framework.process();

		//
		
		audioCallback();
		
		//
		
		framework.beginDraw(0, 0, 0, 0);
		{
			// todo : show source and head position
		}
		framework.endDraw();
	} while (!keyboard.wentDown(SDLK_SPACE));
}
